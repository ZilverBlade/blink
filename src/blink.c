#include "blink.h"

#include "yaml/yaml.h"

#define NEW(type) malloc(sizeof(type))
#define DELETE(data) free((data))
#define CLEAR(data) memset(data, 0, sizeof(*data))

#define BK_KEYWORD_IMPORT "import"
#define BK_KEYWORD_ARGS "args"
#define BK_KEYWORD_STDLIB "std"

char* bk_internal_clone_string(bk_string str) {
    bk_integer len = strlen(str);
    char* new = calloc(len + 1, sizeof(char));
    strncpy(new, str, len);
    return new;
}

struct bk_dynamic_list_t {
    void* data;
    int elementSize;
    int length;
    int capacity;
};
struct bk_dynamic_list_t* bk_internal_create_list(int elementSize, int initialCapacity) {
    struct bk_dynamic_list_t* list = NEW(struct bk_dynamic_list_t);
    CLEAR(list);
    list->data = calloc(initialCapacity, elementSize);
    list->elementSize = elementSize;
    list->capacity = initialCapacity;
    return list;
}
void bk_internal_destroy_list(struct bk_dynamic_list_t* list) {
    DELETE(list->data);
    DELETE(list);
}

void bk_internal_list_append(struct bk_dynamic_list_t* list, const bk_voidptr val) {
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

void bk_internal_list_pop(struct bk_dynamic_list_t* list, bk_voidptr val) {
    --list->length;
    if (val != NULL) {
        memcpy(val, (char*)list->data + list->length * list->elementSize, list->elementSize);
    }
}


struct bk_managed_value_t {
    bk_type type;
    bk_voidptr data;
};

struct bk_managed_value_t* bk_internal_box_managed_value(const bk_voidptr inData, bk_integer size, bk_type type) {
    struct bk_managed_value_t* ret = NEW(struct bk_managed_value_t);
    CLEAR(ret);
    ret->type = type;
    ret->data = size == 0 ? NULL : malloc(size);
    memcpy(ret->data, inData, size);
    return ret;
}

#define bk_internal_box_type(v, type) bk_internal_box_managed_value(&v, sizeof(v), type)

void bk_internal_unbox_managed_value(struct bk_managed_value_t* managedValue, bk_voidptr outData) {
    bk_integer size = 0;
    switch (managedValue->type) {
    case BK_TYPE_UNKNOWN:
        size = 0;
        break;
    case BK_TYPE_INTEGER:
    case BK_TYPE_DECIMAL:
        size = 4;
        break;
    case BK_TYPE_BOOLEAN:
        size = 1;
        break;
    case BK_TYPE_OBJECT:
        size = sizeof(bk_object);
        break;
    case BK_TYPE_STRING:
        size = strlen(managedValue->data);
        break;
    }
    memcpy(outData, managedValue->data, size);
}


void bk_internal_destroy_managed_value(struct bk_managed_value_t* managedValue) {
    DELETE(managedValue->data);
    DELETE(managedValue);
}

struct bk_managed_value_t* bk_internal_parse_as_managed_value(bk_string value) {
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
        char* copy = bk_internal_clone_string(value);
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
    // end of statement
    BK_TOKEN_BREAK,

    // -- GENERIC STATEMENTS --

    BK_TOKEN_IN_START,
    BK_TOKEN_IN_END,

    BK_TOKEN_LIST_START,
    BK_TOKEN_LIST_END,

};

struct bk_debug_source_data_t {
    bk_integer firstCol;
    bk_integer line;
};

struct bk_token_t {
    enum bk_token_type_t type;
    union bk_token_data_t {
        struct bk_token_data_sub_t {
            char* name;
        } sub;
        struct bk_token_data_lit_t {
            struct bk_managed_value_t* value;
        } lit;
    } data;
    struct bk_debug_source_data_t debugSource;
};

struct bk_env_t {
    int x;
};

struct bk_ptr_t {
    int x;
};

struct bk_machine_t {
    yaml_parser_t parser;
    char* lastErrorBuf;
    bk_integer lastErrorMaxBufLen;
};
struct bk_engine_t {
    FILE* ioOut;
    FILE* ioIn;

    struct bk_engine_state_t {
        char* lastErrorBuf;
        bk_integer lastErrorMaxBufLen;
    } state;
};

struct bk_unit_t {
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
    CLEAR(*pResult);
    (*pResult)->ioIn = stdin;
    (*pResult)->ioOut = stdout;
    (*pResult)->state.lastErrorMaxBufLen = 1024;
    (*pResult)->state.lastErrorBuf = calloc((*pResult)->state.lastErrorMaxBufLen, sizeof(char));
    return BK_SUCCESS;
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
    return BK_SUCCESS;
}

void bk_engine_throw_exception(bk_engine engine, bk_exception code, bk_string msg, bk_integer argc, bk_integer* argv) {

}

bk_result bk_engine_create_object(bk_engine engine, bk_metadata metadata, bk_voidptr data, bk_object* pResult) {
    return BK_SUCCESS;
}

bk_string bk_engine_get_error(bk_engine engine) {
    return engine->state.lastErrorBuf;
}

bk_result bk_compile_translation_unit(bk_machine machine, bk_stream* pStream, bk_unit* pResult) {
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
        if (useSubTok != -1) {                              \
            GET_TOKEN(useSubTok)->type = BK_TOKEN_DECL_SUB; \
            useSubTok = -1;                                 \
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
            token.type = BK_TOKEN_IN_START;
            PROMOTE_SUB_USAGE();
            ++nestingLevel;
            break;
        case YAML_MAPPING_END_EVENT:
            token.type = BK_TOKEN_IN_END;
            useSubTok = -1;
            --nestingLevel;
            break;
        case YAML_SEQUENCE_START_EVENT:
            token.type = BK_TOKEN_LIST_START;
            inSequence = BK_TRUE;
            break;
        case YAML_SEQUENCE_END_EVENT:
            token.type = BK_TOKEN_LIST_END;
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
                    token.data.sub.name = bk_internal_clone_string(event.data.scalar.value);
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

    bk_integer length = 0;
    bk_integer remaining = *pBufLen;
    for (bk_integer i = 0; i < unit->tokens->length; ++i) {
#define ADJ_LEN() do if (*pBufLen == 0 || (*pBufLen - length < 0)) remaining = 0; else remaining = *pBufLen - length; while (0)
#define ADJ_BUF() (outBuf ? (outBuf + length) : NULL)
        struct bk_token_t* token = (struct bk_token_t*)unit->tokens->data + i;
        bk_string tokenName = NULL;
        switch (token->type) {
        case BK_TOKEN_DECL_SUB: tokenName = "DECL_SUB"; break;
        case BK_TOKEN_USE_SUB: tokenName = "USE_SUB"; break;
        case BK_TOKEN_USE_LIT: tokenName = "USE_LIT"; break;
        case BK_TOKEN_BREAK: tokenName = "BREAK"; break;
        case BK_TOKEN_IN_START: tokenName = "IN_START"; break;
        case BK_TOKEN_IN_END: tokenName = "IN_END"; break;
        case BK_TOKEN_LIST_START: tokenName = "LIST_START"; break;
        case BK_TOKEN_LIST_END: tokenName = "LIST_END"; break;
        default:
            return BK_FAILURE;
        }
        length += snprintf(ADJ_BUF(), remaining, "bk_token: %s", tokenName);
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
    DELETE(unit);
}

static char s_string_coercion_stack[1024];
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
        memset(s_string_coercion_stack, 0, 1024);
        bk_internal_unbox_managed_value(val, s_string_coercion_stack);
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

#define MAX_ARG_COUNT 64
struct bk_arg_stack_t {
    bk_voidptr stack[MAX_ARG_COUNT];
    bk_integer stackptr;
};
bk_result bk_execute(bk_engine engine, bk_unit* pUnits, bk_integer numUnits, bk_integer entrypoint) {
    if (entrypoint >= numUnits) {
        return BK_ENGINE_FAILURE;
    }
    bk_unit start = pUnits[entrypoint];

    bk_integer tokenIdx = 0;

    bk_integer currentSubToken = -1;

    bk_string prepareFuncCall = NULL;
    bk_boolean inAppendArgs = BK_FALSE;

    struct bk_dynamic_list_t* funcCallArgsStack = bk_internal_create_list(sizeof(struct bk_arg_stack_t), 16);


    bk_result result = BK_SUCCESS;

    while (tokenIdx < start->tokens->length) {
        struct bk_token_t* token = ((struct bk_token_t*)start->tokens->data) + tokenIdx;

        struct bk_arg_stack_t* currStack = ((struct bk_arg_stack_t*)funcCallArgsStack->data) + funcCallArgsStack->length - 1;

        switch (token->type) {
        case BK_TOKEN_DECL_SUB:
            currentSubToken = tokenIdx;
            break;
        case BK_TOKEN_USE_SUB: {
            if (strcmp(token->data.sub.name, "std.writeout") == 0) {
                prepareFuncCall = "std.writeout";
            }
            break;
        }
        case BK_TOKEN_LIST_START: {
            if (prepareFuncCall) {
                inAppendArgs = BK_TRUE;
                bk_internal_list_append(funcCallArgsStack, NULL);
            }
            break;
        }
        case BK_TOKEN_LIST_END: {
            if (prepareFuncCall) {
                if (funcCallArgsStack->length == 0) {
                    snprintf(engine->state.lastErrorBuf, engine->state.lastErrorMaxBufLen,
                        "InternalMalfunctionError at Line %zu Col %zu: Execution engine expected to be in"
                        " a subroutine call but stack is invalid",
                        token->debugSource.line, token->debugSource.firstCol);
                    goto error;
                }
                if (strcmp(prepareFuncCall, "std.writeout") == 0) {
                    if (currStack->stackptr == 0) {
                        snprintf(engine->state.lastErrorBuf, engine->state.lastErrorMaxBufLen,
                            "InsufficientArgumentsError at Line %zu Col %zu: std.writeout expects at least 1 argument, "
                            "but 0 were provided",
                            token->debugSource.line, token->debugSource.firstCol);
                        goto error;
                    }
                    else {
                        bkstdlib_writeout(engine, currStack->stack[0]);
                    }
                }
                currStack->stackptr = 0;
                bk_internal_list_pop(currStack, NULL);
                prepareFuncCall = NULL;
                inAppendArgs = BK_FALSE;
                prepareFuncCall = NULL;
            }
            break;
        }
        case BK_TOKEN_USE_LIT: {
            if (inAppendArgs) {
                if (funcCallArgsStack->length == 0) {
                    snprintf(engine->state.lastErrorBuf, engine->state.lastErrorMaxBufLen,
                        "InternalMalfunctionError at Line %zu Col %zu: Execution engine expected to be in"
                        " a subroutine call but stack is invalid",
                        token->debugSource.line, token->debugSource.firstCol);
                    goto error;
                }
                if (currStack->stackptr == MAX_ARG_COUNT) {
                    snprintf(engine->state.lastErrorBuf, engine->state.lastErrorMaxBufLen,
                        "TooManyArgumentsError at Line %zu Col %zu: Cannot exceed more than 64 arguments when calling a function!",
                        token->debugSource.line, token->debugSource.firstCol);
                    goto error;
                }
                currStack->stack[currStack->stackptr++] = token->data.lit.value;
            }
            break;
        }
        }
        ++tokenIdx;
    }

    return result;

error:
    return BK_ENGINE_FAILURE;
}
