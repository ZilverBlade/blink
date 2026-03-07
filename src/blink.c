#include "blink.h"
#include "internal/strhashmap.h"

#include "internal/yaml/yaml.h"

#include <assert.h>

#define NEW(type) calloc(1, sizeof(type))
#define DELETE(data) free((data))
#define CLEAR(data) memset(data, 0, sizeof(*data))

#define BK_KEYWORD_IMPORT "import"
#define BK_KEYWORD_ARGS "args"
#define BK_KEYWORD_STDLIB "std"

struct bk_dynamic_list_t {
    void* data;
    int elementSize;
    int length;
    int capacity;
};
struct bk_dynamic_list_t* bk_internal_create_list(int elementSize, int initialCapacity) {
    struct bk_dynamic_list_t* list = NEW(struct bk_dynamic_list_t);
    list->data = calloc(initialCapacity, elementSize);
    list->elementSize = elementSize;
    list->capacity = initialCapacity;
    return list;
}
static void bk_internal_destroy_list(struct bk_dynamic_list_t* list) {
    DELETE(list->data);
    DELETE(list);
}

static void bk_internal_list_append(struct bk_dynamic_list_t* list, const bk_voidptr val) {
    if (list->length == list->capacity) {
        list->capacity *= 2;
        list->data = realloc(list->data, list->capacity * list->elementSize);
        memset((char*)list->data + list->length, 0, (list->capacity - list->length) * list->elementSize);
    }
    if (val != NULL) {
        memcpy((char*)list->data + list->length * list->elementSize, val, list->elementSize);
    }
    list->length++;
}

static  void bk_internal_list_pop(struct bk_dynamic_list_t* list, bk_voidptr val) {
    --list->length;
    if (val != NULL) {
        memcpy(val, (char*)list->data + list->length * list->elementSize, list->elementSize);
    }
}


static void bk_internal_list_clear(struct bk_dynamic_list_t* list) {
    DELETE(list->data);
    list->data = calloc(list->capacity, list->elementSize);
    list->length = 0;
}


struct bk_managed_value_t {
    bk_type type;
    bk_voidptr data;
};


struct bk_managed_value_t* bk_internal_box_managed_value(const bk_voidptr inData, bk_integer size, bk_type type) {
    struct bk_managed_value_t* ret = NEW(struct bk_managed_value_t);
    ret->type = type;
    ret->data = size == 0 ? NULL : malloc(size);
    memcpy(ret->data, inData, size);
    return ret;
}

#define bk_internal_box_type(v, type) bk_internal_box_managed_value(&v, sizeof(v), type)

static bk_integer bk_internal_get_managed_value_size(struct bk_managed_value_t* managedValue) {
    switch (managedValue->type) {
    case BK_TYPE_UNKNOWN:
        return 0;
    case BK_TYPE_INTEGER:
    case BK_TYPE_DECIMAL:
    case BK_TYPE_BOOLEAN:
        return 4;
    case BK_TYPE_OBJECT:
        return sizeof(bk_object);
    case BK_TYPE_STRING:
        return strlen(managedValue->data);
        break;
    default:
        assert(0);
    }
}
struct bk_managed_value_t* bk_internal_clone_managed_value(struct bk_managed_value_t* managedValue) {
    return bk_internal_box_managed_value(managedValue->data, bk_internal_get_managed_value_size(managedValue), managedValue->type);
}

static void bk_internal_unbox_managed_value(struct bk_managed_value_t* managedValue, bk_voidptr outData) {
    assert(outData);
    bk_integer size = bk_internal_get_managed_value_size(managedValue);
    memcpy(outData, managedValue->data, size);
}


static void bk_internal_destroy_managed_value(struct bk_managed_value_t* managedValue) {
    if (managedValue) {
        DELETE(managedValue->data);
    }
    DELETE(managedValue);
}

static struct bk_managed_value_t* bk_internal_parse_as_managed_value(bk_string value) {
    if (!value || value[0] == '\0' || strcmp(value, "unknown") == 0) {
        return bk_internal_box_managed_value(NULL, 0, BK_TYPE_UNKNOWN);
    }
    {
        char* ptr = NULL;
        double num = strtod(value, &ptr);
        if (errno != ERANGE && ptr > value) {
            if (num != (double)(long)num) {
                bk_decimal v = (bk_decimal)num;
                return bk_internal_box_type(v, BK_TYPE_DECIMAL);
            }
            else {
                bk_integer v = (bk_integer)num;
                return bk_internal_box_type(v, BK_TYPE_INTEGER);
            }
        }
    }
    {
        char* copy = strdup(value);
        char* trimmed = copy;
        while (trimmed[0] == ' ') {
            ++trimmed;
        }

        for (char* b = trimmed; *b != '\0'; ++b) {
            if (*b >= 'A' && *b <= 'Z') {
                *b += 'a' - 'A';
            }
        }

        bk_boolean bool;
        bk_boolean exists = BK_FALSE;
        if (strcmp(trimmed, "y") == 0 || strcmp(trimmed, "yes") == 0 ||
            strcmp(trimmed, "true") == 0 || strcmp(trimmed, "on") == 0) {
            bool = BK_TRUE;
            exists = BK_TRUE;
        }
        else if (strcmp(trimmed, "n") == 0 || strcmp(trimmed, "no") == 0 ||
            strcmp(trimmed, "false") == 0 || strcmp(trimmed, "off") == 0) {
            bool = BK_FALSE;
            exists = BK_TRUE;
        }
        DELETE(copy);
        if (exists) {
            return bk_internal_box_type(bool, BK_TYPE_BOOLEAN);
        }
    }

    return NULL;
}

enum bk_token_type_t {
    BK_TOKEN_NONE = 0,
    // -- SPECIAL STATEMENTS --
    // declare a subroutine
    BK_TOKEN_DECL_SUB,
    // subroutine usage
    BK_TOKEN_USE_SUB,
    // literal usage (e.g. string literal, number)
    BK_TOKEN_USE_LIT,

    // -- GENERIC STATEMENTS --

    BK_TOKEN_SUB_START,
    BK_TOKEN_SUB_END,

    BK_TOKEN_ARG_START,
    BK_TOKEN_ARG_END,

};

struct bk_debug_source_data_t {
    bk_integer firstCol;
    bk_integer line;
};

struct bk_token_t {
    enum bk_token_type_t type;
    union bk_token_data_t {
        struct bk_token_data_sub_t {
            bk_string name;
        } sub;
        struct bk_token_data_lit_t {
            struct bk_managed_value_t* value;
        } lit;
    } data;
    struct bk_debug_source_data_t debugSource;
};

struct bk_sub_t {
    bk_integer tokStart;
    bk_integer tokEnd;
    bk_integer argc;
    struct bk_managed_value_t* currentValue;
    bk_string alias;
    bk_boolean isArgumentInput;
};

static void bk_internal_destroy_sub(struct bk_sub_t* sub) {
    if (sub) {
        DELETE(sub->alias);
        bk_internal_destroy_managed_value(sub->currentValue);
    }
    DELETE(sub);
}

struct bk_machine_t {
    yaml_parser_t parser;
    char* lastErrorBuf;
    bk_integer lastErrorMaxBufLen;
};

struct bk_engine_unit_state_t {
    bk_strhashmap subs; // struct bk_sub_t
    struct bk_managed_value_t* currentValue;
};
struct bk_engine_state_t {
    char* lastErrorBuf;
    bk_integer lastErrorMaxBufLen;
    bk_strhashmap unitStates; // struct bk_engine_unit_state_t
};
struct bk_engine_t {
    FILE* ioOut;
    FILE* ioIn;
    struct bk_engine_state_t state;
};

struct bk_unit_t {
    bk_string name;
    struct bk_dynamic_list_t* tokens;
};

bk_result bk_create_interpreter(bk_machine* pResult) {
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        return BK_INIT_FAILURE;
    }
    *pResult = NEW(struct bk_machine_t);
    (*pResult)->parser = parser;
    (*pResult)->lastErrorMaxBufLen = 1024;
    (*pResult)->lastErrorBuf = calloc((*pResult)->lastErrorMaxBufLen, sizeof(char));
    return BK_SUCCESS;
}

void bk_destroy_interpreter(bk_machine machine) {
    if (machine == NULL) return;
    yaml_parser_delete(&machine->parser);
    DELETE(machine->lastErrorBuf);
    DELETE(machine);
}

bk_string bk_interpreter_get_error(bk_machine machine) {
    if (machine == NULL) {
        return NULL;
    }
    return machine->lastErrorBuf;
}

bk_result bk_interpreter_attach_library(bk_machine machine, bk_voidptr library) {
    return BK_SUCCESS;
}

bk_result bk_create_execution_engine(bk_engine* pResult) {
    *pResult = NEW(struct bk_engine_t);
    (*pResult)->ioIn = stdin;
    (*pResult)->ioOut = stdout;
    return bk_engine_reset_state(*pResult);
}

void bk_destroy_execution_engine(bk_engine engine) {
    DELETE(engine->state.lastErrorBuf);
    DELETE(engine);
}

bk_result bk_engine_set_io(bk_engine engine, FILE* input, FILE* output) {
    if (engine == NULL) return BK_NULL_FAILURE;
    engine->ioIn = input;
    engine->ioOut = output;
    return BK_SUCCESS;
}

bk_result bk_engine_get_io(bk_engine engine, FILE** pInput, FILE** pOutput) {
    if (engine == NULL) return BK_NULL_FAILURE;
    if (pInput) *pInput = engine->ioIn;
    if (pOutput) *pOutput = engine->ioOut;
    return BK_SUCCESS;
}

bk_result bk_engine_reset_state(bk_engine engine) {
    bk_internal_strhashmap_for_each(engine->state.unitStates, it) {
        struct bk_engine_unit_state_t* unitstate = it.value;
        bk_internal_strhashmap_for_each(unitstate->subs, it2) {
            bk_internal_destroy_sub(it2.value); // struct bk_sub_t
        }
        free(unitstate);
    }
    bk_internal_free_strhashmap(engine->state.unitStates);
    engine->state.unitStates = bk_internal_create_strhashmap();

    engine->state.lastErrorMaxBufLen = 1024;
    DELETE(engine->state.lastErrorBuf);
    engine->state.lastErrorBuf = calloc(engine->state.lastErrorMaxBufLen, sizeof(char));

    return BK_SUCCESS;
}

void bk_engine_throw_exception(bk_engine engine, bk_exception code, bk_string msg, bk_integer argc, bk_integer* argv) {
    fprintf(stderr, "Exception %0xi: %s\n", (int)code, msg);
    assert(0);
}

bk_result bk_engine_create_object(bk_engine engine, bk_metadata metadata, bk_voidptr data, bk_object* pResult) {
    return BK_SUCCESS;
}

bk_string bk_engine_get_error(bk_engine engine) {
    return engine->state.lastErrorBuf;
}

bk_result bk_compile_translation_unit(bk_machine machine, bk_stream* pStream, bk_string name, bk_unit* pResult) {
    if (name == NULL) {
        return BK_NULL_FAILURE;
    }
    yaml_event_t event;

    yaml_parser_set_input_file(&machine->parser, pStream->pFile);

    struct bk_dynamic_list_t* tokens = bk_internal_create_list(sizeof(struct bk_token_t), 64);

    bk_boolean done = BK_FALSE;
    bk_result result = BK_SUCCESS;

    bk_integer nestingLevel = -1;
    bk_integer useSubTok = -1;
    bk_boolean inSequence = BK_FALSE;

    /* Read the event sequence. */
    while (!done) {
        /* Get the next event. */
        if (!yaml_parser_parse(&machine->parser, &event)) {
            if (machine->parser.error == YAML_PARSER_ERROR || machine->parser.error == YAML_SCANNER_ERROR) {
                snprintf(machine->lastErrorBuf, machine->lastErrorMaxBufLen,
                    "Parser error (%d) at Line %zu Col %zu: %s",
                    machine->parser.problem_value, machine->parser.problem_mark.line + 1, machine->parser.problem_mark.column,
                    machine->parser.problem);
                result = BK_PARSER_ERROR;
                goto error;
            }
            if (machine->parser.error == YAML_READER_ERROR) {
                snprintf(machine->lastErrorBuf, machine->lastErrorMaxBufLen,
                    "IO error (%d): %s",
                    machine->parser.problem_value, machine->parser.problem);
                result = BK_IO_ERROR;
                goto error;
            }
            result = BK_FAILURE;
            goto error;
        }
        struct bk_token_t token;
        CLEAR(&token);

#define GET_TOKEN(i) ((struct bk_token_t*)tokens->data + i)

#define PROMOTE_SUB_USAGE() do\
        if (useSubTok != -1) {                                      \
            struct bk_token_t* pred = GET_TOKEN((useSubTok - 1));   \
            if (!(pred != NULL && pred->type == BK_TOKEN_DECL_SUB)) \
                GET_TOKEN(useSubTok)->type = BK_TOKEN_DECL_SUB;     \
            useSubTok = -1;                                         \
        } while (0)

        switch (event.type) {
        case YAML_STREAM_START_EVENT:
            done = BK_FALSE;
            break;
        case YAML_STREAM_END_EVENT:
            done = BK_FALSE;
            break;
        case YAML_DOCUMENT_START_EVENT:
            break;
        case YAML_DOCUMENT_END_EVENT:
            break;

        case YAML_MAPPING_START_EVENT:
            token.type = BK_TOKEN_SUB_START;
            PROMOTE_SUB_USAGE();
            ++nestingLevel;
            break;
        case YAML_MAPPING_END_EVENT:
            token.type = BK_TOKEN_SUB_END;
            useSubTok = -1;
            --nestingLevel;
            break;
        case YAML_SEQUENCE_START_EVENT:
            token.type = BK_TOKEN_ARG_START;
            inSequence = BK_TRUE;
            break;
        case YAML_SEQUENCE_END_EVENT:
            token.type = BK_TOKEN_ARG_END;
            inSequence = BK_FALSE;
            useSubTok = -1;
            break;

        case YAML_ALIAS_EVENT:
            break;
        case YAML_SCALAR_EVENT:
            if (!inSequence) {
                PROMOTE_SUB_USAGE();
            }
            if (event.data.scalar.style == YAML_DOUBLE_QUOTED_SCALAR_STYLE || event.data.scalar.style == YAML_SINGLE_QUOTED_SCALAR_STYLE) {
                token.type = BK_TOKEN_USE_LIT;
                token.data.lit.value = bk_internal_box_managed_value(event.data.scalar.value,
                    event.data.scalar.length + 1, BK_TYPE_STRING);
            }
            else {
                token.type = BK_TOKEN_USE_LIT;
                token.data.lit.value = bk_internal_parse_as_managed_value(event.data.scalar.value);
                if (!token.data.lit.value) {
                    // must be a sub usage or declaration! This will depend on the following arguments
                    token.type = BK_TOKEN_USE_SUB;
                    token.data.sub.name = strdup(event.data.scalar.value);
                    useSubTok = tokens->length;
                }
            }

            break;
        default:
            break;
        }

        /* Are we finished? */
        done = (event.type == YAML_STREAM_END_EVENT);

        if (token.type != BK_TOKEN_NONE) {
            token.debugSource.line = event.start_mark.line + 1;
            token.debugSource.firstCol = event.start_mark.column;
            bk_internal_list_append(tokens, &token);
        }
        yaml_event_delete(&event);
    }

    *pResult = NEW(struct bk_unit_t);
    (*pResult)->tokens = tokens;
    (*pResult)->name = strdup(name);
    return BK_SUCCESS;

error:
    yaml_event_delete(&event);
    bk_internal_destroy_list(tokens);
    return result;
}

bk_result bk_emit_translation_unit(bk_unit unit, char* outBuf, bk_integer* pBufLen) {
    if (unit == NULL || pBufLen == NULL) {
        return BK_NULL_FAILURE;
    }
#define ADJ_LEN() do if (*pBufLen == 0 || (*pBufLen - length < 0)) remaining = 0; else remaining = *pBufLen - length; while (0)
#define ADJ_BUF() (outBuf ? (outBuf + length) : NULL)

    bk_integer length = 0;
    bk_integer remaining = *pBufLen;

    length += snprintf(ADJ_BUF(), remaining, "bk_unit: %s\n", unit->name);
    ADJ_LEN();

    for (bk_integer i = 0; i < unit->tokens->length; ++i) {
        struct bk_token_t* token = (struct bk_token_t*)unit->tokens->data + i;
        bk_string tokenName = NULL;
        switch (token->type) {
        case BK_TOKEN_DECL_SUB: tokenName = "DECL_SUB"; break;
        case BK_TOKEN_USE_SUB: tokenName = "USE_SUB"; break;
        case BK_TOKEN_USE_LIT: tokenName = "USE_LIT"; break;
        case BK_TOKEN_SUB_START: tokenName = "SUB_START"; break;
        case BK_TOKEN_SUB_END: tokenName = "SUB_END"; break;
        case BK_TOKEN_ARG_START: tokenName = "ARG_START"; break;
        case BK_TOKEN_ARG_END: tokenName = "ARG_END"; break;
        default:
            return BK_FAILURE;
        }
        length += snprintf(ADJ_BUF(), remaining, "%d\t| bk_token: %s", i, tokenName);
        ADJ_LEN();
        switch (token->type) {
        case BK_TOKEN_DECL_SUB:
        case BK_TOKEN_USE_SUB:
            length += snprintf(ADJ_BUF(), remaining, ", (%s)", token->data.sub.name);
            break;
        case BK_TOKEN_USE_LIT:
            switch (token->data.lit.value->type) {
            case BK_TYPE_OBJECT:
                length += snprintf(ADJ_BUF(), remaining, ", [%p]", token->data.lit.value->data);
                break;
            case BK_TYPE_STRING:
                length += snprintf(ADJ_BUF(), remaining, ", [\"%s\"]", (bk_string)token->data.lit.value->data);
                break;
            case BK_TYPE_BOOLEAN:
                length += snprintf(ADJ_BUF(), remaining, ", [%s]", *(bk_boolean*)token->data.lit.value->data ? "True" : "False");
                break;
            case BK_TYPE_INTEGER:
                length += snprintf(ADJ_BUF(), remaining, ", [%i]", *(bk_integer*)token->data.lit.value->data);
                break;
            case BK_TYPE_DECIMAL:
                length += snprintf(ADJ_BUF(), remaining, ", [%f]", *(bk_decimal*)token->data.lit.value->data);
                break;
            case BK_TYPE_UNKNOWN:
                length += snprintf(ADJ_BUF(), remaining, ", [unknown]");
                break;
            default:
                return BK_FAILURE;
            }
        default:
            break;
        }
        ADJ_LEN();
        length += snprintf(ADJ_BUF(), remaining, "\n");
        ADJ_LEN();
#undef ADJ_LEN
    }
    if (!outBuf) {
        *pBufLen = length;
    }
    return BK_SUCCESS;
}

void bk_release_translation_unit(bk_unit unit) {
    if (unit == NULL) return;
    if (unit->tokens != NULL) {
        for (bk_integer i = 0; i < unit->tokens->length; ++i) {
            struct bk_token_t* token = (struct bk_token_t*)unit->tokens->data + i;
            switch (token->type) {
            case BK_TOKEN_DECL_SUB:
            case BK_TOKEN_USE_SUB:
                DELETE(token->data.sub.name);
                break;
            case BK_TOKEN_USE_LIT:
                bk_internal_destroy_managed_value(token->data.lit.value);
                break;
            default:
                break;
            }
        }
        bk_internal_destroy_list(unit->tokens);
    }
    DELETE(unit->name);
    DELETE(unit);
}

#define cs_string_coercion_stack 1024
static char s_string_coercion_stack[cs_string_coercion_stack];
static void bk_internal_coerce_to_string(struct bk_managed_value_t* val, char** pStr) {
    if (val->type == BK_TYPE_UNKNOWN) {
        memset(s_string_coercion_stack, 0, 2);
    }
    else if (val->type == BK_TYPE_BOOLEAN) {
        memset(s_string_coercion_stack, 0, 10);
        bk_boolean flag;
        bk_internal_unbox_managed_value(val, &flag);
        strncpy(s_string_coercion_stack, flag ? "True" : "False", 10);
    }
    else if (val->type == BK_TYPE_STRING) {
        memset(s_string_coercion_stack, 0, cs_string_coercion_stack);
        bk_internal_unbox_managed_value(val, s_string_coercion_stack);
    }
    else if (val->type == BK_TYPE_INTEGER) {
        memset(s_string_coercion_stack, 0, cs_string_coercion_stack);
        bk_integer value;
        bk_internal_unbox_managed_value(val, &value);
        snprintf(s_string_coercion_stack, cs_string_coercion_stack, "%i", value);
    }
    else if (val->type == BK_TYPE_DECIMAL) {
        memset(s_string_coercion_stack, 0, cs_string_coercion_stack);
        bk_decimal value;
        bk_internal_unbox_managed_value(val, &value);
        snprintf(s_string_coercion_stack, cs_string_coercion_stack, "%f", value);
    }
    *pStr = s_string_coercion_stack;
}

static void bkstdlib_writeout(bk_engine engine, struct bk_managed_value_t* fmt) {
    FILE* out;
    if (bk_engine_get_io(engine, NULL, &out) != BK_SUCCESS) {
        bk_engine_throw_exception(engine, BK_EX_ENGINE_FAILURE, "Failed to get IO!", 0, NULL);
        return;
    }
    if (!out) {
        bk_engine_throw_exception(engine, BK_EX_BADIO, "Missing output FILE", 0, NULL);
        return;
    };

    char* ptr;
    bk_internal_coerce_to_string(fmt, &ptr);
    fprintf(out, "%s\n", ptr);
}

static bk_result bk_gather_arguments(bk_engine engine, struct bk_engine_unit_state_t* unitState, bk_unit unit,
    bk_integer* pTokenIdx, struct bk_dynamic_list_t* argList
) {
    bk_boolean inArgs = BK_FALSE;
    bk_result result = BK_SUCCESS;
    bk_integer currArgIdx = 0;
    while (*pTokenIdx < unit->tokens->length) {
        // break when end args is found

        bk_integer currTokenIdx = (*pTokenIdx)++;
        struct bk_token_t* token = ((struct bk_token_t*)unit->tokens->data) + currTokenIdx;
        switch (token->type) {
        case BK_TOKEN_ARG_START:
            if (inArgs) {
                result = BK_INTERP_ERROR;
                goto error;
            }
            inArgs = BK_TRUE;
            break;
        case BK_TOKEN_ARG_END:
            if (!inArgs) {
                result = BK_INTERP_ERROR;
                goto error;
            }
            inArgs = BK_FALSE;
            return BK_SUCCESS;
            break;
        case BK_TOKEN_USE_SUB: {
            struct bk_sub_t* argSub = NEW(struct bk_sub_t);
            argSub->isArgumentInput = BK_TRUE;
            struct bk_sub_t* oldSub = bk_internal_strhashmap_put(unitState->subs, token->data.sub.name, argSub);
            bk_internal_destroy_sub(oldSub);
            // get pushed argument value
            if (currArgIdx >= argList->length) {
                bk_engine_throw_exception(engine, BK_EX_MISSINGARGUMENT, "Too little arguments provided!", 0, NULL);
                goto error;
            }
            struct bk_managed_value_t** pushedArg = argList->data;
            pushedArg += currArgIdx++;
            argSub->currentValue = *pushedArg;
            CLEAR(pushedArg);
            break;
        }
        default:
            result = BK_INTERP_ERROR;
            goto error;
        }
    }
error:
    return result;
}

static bk_result bk_push_args(bk_engine engine, struct bk_engine_unit_state_t* unitState, bk_unit unit,
    bk_integer* pTokenIdx, struct bk_dynamic_list_t** pArgList) {
    bk_boolean inArgs = BK_FALSE;
    bk_result result = BK_SUCCESS;
    bk_integer currArgIdx = 0;
    struct bk_dynamic_list_t* localArgList = bk_internal_create_list(sizeof(struct bk_managed_value_t*), 8);

    struct bk_sub_t* pendingSubCall = NULL;
    while (*pTokenIdx < unit->tokens->length) {
        // break when end args is found

        bk_integer currTokenIdx = (*pTokenIdx)++;
        struct bk_token_t* token = ((struct bk_token_t*)unit->tokens->data) + currTokenIdx;
        switch (token->type) {
        case BK_TOKEN_ARG_START:
            if (pendingSubCall != NULL) {
                // allow for nested sub usages!
                bk_push_args(engine, unitState, unit, pTokenIdx, &localArgList);
                bk_integer subTokIdx = pendingSubCall->tokStart;
                struct bk_sub_t dummySub;
                CLEAR(&dummySub);
                result = bk_consume_sub(engine, unitState, unit, &subTokIdx, pendingSubCall->tokEnd, &dummySub, localArgList);
                if (result != BK_SUCCESS) {
                    goto cleanup;
                }
                bk_internal_list_append(*pArgList, &dummySub.currentValue);
            }
        case BK_TOKEN_ARG_END:
            inArgs = BK_FALSE;
            --*pTokenIdx;
            goto cleanup;
        case BK_TOKEN_USE_SUB:
            --*pTokenIdx;
            pendingSubCall = bk_internal_strhashmap_get_ptr(
                unitState->subs, token->data.sub.name);
            break;
        case BK_TOKEN_USE_LIT: {
            void* data = bk_internal_clone_managed_value(token->data.lit.value);
            bk_internal_list_append(*pArgList, &data);
            break;
        }
        default:
            result = BK_INTERP_ERROR;
            goto cleanup;
        }
    }
cleanup:
    bk_internal_destroy_list(localArgList);
    return result;
}

static bk_result bk_consume_sub(bk_engine engine, struct bk_engine_unit_state_t* unitState, bk_unit unit,
    bk_integer* pTokenIdx, bk_integer endTokenIdx, struct bk_sub_t* currSub, struct bk_dynamic_list_t* argList) {
    bk_result result = BK_SUCCESS;
    struct bk_managed_value_t** pReturn = NULL;
    if (currSub == NULL) {
        pReturn = &unitState->currentValue;
    }
    else {
        assert(!currSub->isArgumentInput && "Do not call argument inputs! They are already evaluated!");
        pReturn = &currSub->currentValue;
    }
    struct bk_sub_t* lastSubDecl = NULL;
    struct bk_sub_t* pendingSubUse = NULL;
    bk_integer local_scope_block = 0;
    bk_integer pending_scope_block = -1;
    bk_boolean previous_token_is_subdecl = BK_FALSE;
    bk_boolean gathered = BK_FALSE;

#define UPDATE_D()    { previous_token_is_subdecl = BK_FALSE;                           \
    if (pending_scope_block != -1 && local_scope_block != pending_scope_block) continue;\
    pending_scope_block = -1;                                                           \
    if (lastSubDecl) { /* parent sub */                                                 \
        lastSubDecl->tokEnd = currTokenIdx;                                             \
        lastSubDecl = NULL;                                                             \
    }                                                                                   \
    }          

    while (*pTokenIdx < endTokenIdx) {
        bk_integer currTokenIdx = (*pTokenIdx)++;
        struct bk_token_t* token = ((struct bk_token_t*)unit->tokens->data) + currTokenIdx;
        switch (token->type) {
        case BK_TOKEN_DECL_SUB: {
            UPDATE_D();
            currSub = NEW(struct bk_sub_t);
            lastSubDecl = currSub;
            currSub->tokStart = currTokenIdx + 1;
            struct bk_sub_t* oldSub = bk_internal_strhashmap_put(unitState->subs, token->data.sub.name, currSub);
            if (oldSub) {
                currSub->currentValue = oldSub->currentValue;
                // take ownership
                oldSub->currentValue = NULL;
            }
            else { //default to unknown
                currSub->currentValue = bk_internal_box_managed_value(NULL, 0, BK_TYPE_UNKNOWN);
            }
            pending_scope_block = local_scope_block;
            previous_token_is_subdecl = BK_TRUE;
            bk_internal_destroy_sub(oldSub);
            break;
        } case BK_TOKEN_USE_SUB:
            if (previous_token_is_subdecl) { // alias!
                struct bk_sub_t* aliasor = bk_internal_strhashmap_get_ptr(unitState->subs, (token - 1)->data.sub.name);
                assert(aliasor);
                DELETE(aliasor->alias);
                aliasor->alias = strdup(token->data.sub.name);
                previous_token_is_subdecl = BK_FALSE;
                continue;
            }
            UPDATE_D();
            if (strcmp(token->data.sub.name, BK_KEYWORD_IMPORT) == 0) {
                result = BK_ENGINE_NOT_IMPLEMENTED;
                goto error;
                //bk_find_imports(engine, unitState, unit, pTokenIdx);
                //currSub->currentValue = bk_internal_box_managed_value(NULL, 0, BK_TYPE_UNKNOWN);
            }
            else if (strcmp(token->data.sub.name, BK_KEYWORD_ARGS) == 0) {
                if (gathered) {
                    bk_engine_throw_exception(engine, BK_EX_DOUBLEGATHER, "Gathered arguments twice!", 0, NULL);
                    result = BK_ENGINE_EXCEPTION;
                    goto error;
                }
                bk_gather_arguments(engine, unitState, unit, pTokenIdx, argList);
                gathered = BK_TRUE;
                bk_internal_list_clear(argList);
            }
            else {
                pendingSubUse = bk_internal_strhashmap_get_ptr(unitState->subs, token->data.sub.name);
                if (pendingSubUse == NULL) {
                    bk_engine_throw_exception(engine, BK_EX_UNKNOWNSUB, "Sub does not exist!", 0, NULL);
                }
                bk_string alias = pendingSubUse->alias;
                while (alias != NULL) {
                    pendingSubUse = bk_internal_strhashmap_get_ptr(unitState->subs, pendingSubUse->alias);
                    assert(pendingSubUse && "aliased sub must have been valid!");
                    alias = pendingSubUse->alias;
                }
            }
            break;
        case BK_TOKEN_USE_LIT:
            if (previous_token_is_subdecl) {
                continue;
            }
            else {
                UPDATE_D();
            }
            bkstdlib_writeout(engine, token->data.lit.value);
            bk_internal_destroy_managed_value(*pReturn);
            *pReturn = bk_internal_clone_managed_value(token->data.lit.value);
            previous_token_is_subdecl = BK_FALSE;
            break;
        case BK_TOKEN_ARG_START:
            UPDATE_D();
            if (pendingSubUse) {
                bk_push_args(engine, unitState, unit, pTokenIdx, &argList);
            }
            else {
                result = BK_INTERP_ERROR;
                goto error;
            }
            break;
        case BK_TOKEN_ARG_END:
            UPDATE_D();
            if (pendingSubUse) {
                if (pendingSubUse->isArgumentInput) {
                    // Short-circuit: arguments don't need token unrolling
                    bk_internal_destroy_managed_value(*pReturn);
                    *pReturn = bk_internal_clone_managed_value(pendingSubUse->currentValue);
                }
                else {
                    // Standard unroll for standard subroutines
                    bk_integer subToken = pendingSubUse->tokStart;
                    assert(subToken > 0 && "Bad start token!");

                    result = bk_consume_sub(engine, unitState, unit, &subToken,
                        pendingSubUse->tokEnd, currSub, argList);
                    if (result != BK_SUCCESS) {
                        goto error;
                    }

                    struct bk_token_t* subTokenPtr = unit->tokens->data;
                    subTokenPtr += pendingSubUse->tokStart - 1;
                    struct bk_sub_t* calledSub = bk_internal_strhashmap_get_ptr(
                        unitState->subs, subTokenPtr->data.sub.name);
                    bk_internal_destroy_managed_value(*pReturn);
                    *pReturn = bk_internal_clone_managed_value(calledSub->currentValue);
                }
                assert(*pReturn);
                pendingSubUse = NULL;
            }
            else {
                result = BK_INTERP_ERROR;
                goto error;
            }
            break;
        case BK_TOKEN_SUB_START:
            previous_token_is_subdecl = BK_FALSE;
            ++local_scope_block;
            break;
        case BK_TOKEN_SUB_END:
            previous_token_is_subdecl = BK_FALSE;
            --local_scope_block;
            break;
        default:
            break;
        }
    }

error:

    return result;
}
bk_result bk_execute(bk_engine engine, bk_unit* pUnits, bk_integer numUnits, bk_integer entrypoint) {
    if (entrypoint >= numUnits) {
        return BK_ENGINE_FAILURE;
    }
    for (bk_integer i = 0; i < numUnits; ++i) {
        if (!bk_internal_strhashmap_get_ptr(engine->state.unitStates, pUnits[i]->name)) {
            struct bk_engine_unit_state_t* unitState = NEW(struct bk_engine_unit_state_t);
            bk_internal_strhashmap_put(engine->state.unitStates, pUnits[i]->name, unitState);
            unitState->subs = bk_internal_create_strhashmap();
        }
    }

    bk_result result = BK_SUCCESS;
    bk_unit start = pUnits[entrypoint];
    struct bk_engine_unit_state_t* currUnitState = bk_internal_strhashmap_get_ptr(engine->state.unitStates, start->name);
    assert(currUnitState);
    bk_integer tokenIdx = 0;
    struct bk_dynamic_list_t* dummyList = bk_internal_create_list(sizeof(struct bk_managed_value_t*), 1);

    result = bk_consume_sub(engine,
        currUnitState, start,
        &tokenIdx, start->tokens->length, NULL, dummyList);

    bk_internal_destroy_list(dummyList);
    if (result == BK_SUCCESS) {
        bkstdlib_writeout(engine, currUnitState->currentValue);
    }
    else {
        fprintf(stderr, "ENGINE FAILURE: %s\n", engine->state.lastErrorBuf);
    }
    return result;

error:
    return BK_ENGINE_FAILURE;
}
