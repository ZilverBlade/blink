#include "blink.h"

#include "yaml/yaml.h"

#define NEW(type) malloc(sizeof(type)); 
#define DELETE(data) free((void*)(data))
#define CLEAR(data) memset((void*)(data), 0, sizeof(*data))

#define BK_KEYWORD_IMPORT "import"
#define BK_KEYWORD_ARGS "args"

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
void bk_internal_destroy_list(struct bk_dynamic_list_t* list) {
    DELETE(list->data);
    DELETE(list);
}

void bk_internal_list_append(struct bk_dynamic_list_t* list, const void* val) {
    if (list->length == list->capacity) {
        list->capacity *= 2;
        list->data = realloc(list->data, list->capacity);
    }
    memcpy((char*)list->data + list->length++ * list->elementSize, val, list->elementSize);
}


struct bk_managed_value_t {
    bk_type type;
    bk_object data;
};

struct bk_managed_value_t* bk_internal_box_managed_value(const bk_object inData, bk_integer size, bk_type type) {
    struct bk_managed_value_t* ret = NEW(struct bk_managed_value_t);
    CLEAR(ret);
    ret->type = type;
    ret->data = malloc(size);
    memcpy(ret->data, inData, size);
    return ret;
}

#define bk_internal_box_type(v, type) bk_internal_box_managed_value(&v, sizeof(v), type)

void bk_internal_unbox_managed_value(struct bk_managed_value_t** pManagedValue, bk_object outData) {
    bk_integer size = 0;
    switch ((*pManagedValue)->type) {
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
        size = sizeof(bk_string);
        break;
    }

    memcpy(outData, (*pManagedValue)->data, size);

    DELETE(*pManagedValue);
    *pManagedValue = NULL;
}

struct bk_managed_value_t* bk_internal_parse_as_managed_value(bk_string value) {
    {
        char* ptr = NULL;
        bk_integer num = strtol(value, &ptr, 10);
        if (errno != ERANGE && ptr > value) {
            return bk_internal_box_type(num, BK_TYPE_INTEGER);
        }
    }
    {
        char* ptr = NULL;
        float num = strtof(value, &ptr, 10);
        if (errno != ERANGE && ptr > value) {
            return bk_internal_box_type(num, BK_TYPE_DECIMAL);
        }
    }
}

enum bk_token_type_t {
    // -- SPECIAL STATEMENTS --

    // declare imports
    BK_TOKEN_DECL_IMPORT,
    // declare a subroutine
    BK_TOKEN_DECL_SUB,
    // declare arguments (only valid in the top of a subroutine declaration)
    BK_TOKEN_DECL_ARGS,
    // declare variable
    BK_TOKEN_DECL_VAR,
    // subroutine usage
    BK_TOKEN_USE_SUB,
    // variable usage
    BK_TOKEN_USE_VAR,
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

struct bk_token_t {
    enum bk_token_type_t type;
    union bk_token_data_t {
        struct bk_token_data_sub_t {
            bk_string name;
        } sub;
        struct bk_token_data_var_t {
            bk_string name;
            struct bk_managed_value_t* value;
        } var;
        struct bk_token_data_lit_t {
            struct bk_managed_value_t* value;
        } lit;
    } data;
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
    bk_integer lastErrorBufLen;
};
struct bk_engine_t {
    FILE* ioOut;
    FILE* ioIn;

    struct bk_engine_state_t {
        int x;
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
    *pResult = NEW(bk_machine);
    (*pResult)->parser = parser;
    (*pResult)->lastErrorBufLen = 1024;
    (*pResult)->lastErrorBuf = calloc((*pResult)->lastErrorBufLen, sizeof(char));
    return BK_SUCCESS;
}

void bk_destroy_interpreter(bk_machine machine) {
    if (machine == NULL) return;
    yaml_parser_delete(&machine->parser);
    DELETE(machine->lastErrorBuf);
    DELETE(machine);
}

bk_string bk_get_interpreter_error(bk_machine machine) {
    if (machine == NULL) {
        return NULL;
    }
    return machine->lastErrorBuf;
}

bk_result bk_create_execution_engine(bk_engine* pResult) {
    *pResult = NEW(bk_engine);
    CLEAR(*pResult);
    return BK_SUCCESS;
}

void bk_destroy_execution_engine(bk_engine engine) {
    DELETE(engine);
}

bk_result bk_set_execution_engine_io(bk_engine engine, FILE* input, FILE* output) {
    if (engine == NULL) return BK_NULL_FAILURE;
    engine->ioIn = input;
    engine->ioOut = output;
    return BK_SUCCESS;
}

bk_result bk_reset_execution_engine_state(bk_engine engine) {
    return BK_SUCCESS;
}

bk_result bk_create_translation_unit(bk_machine machine, const bk_stream* pStream, bk_unit* pResult) {
    yaml_event_t event;

    yaml_parser_set_input_file(&machine->parser, pStream->pFile);

    struct bk_dynamic_list_t* tokens = bk_internal_create_list(sizeof(struct bk_token_t), 64);


    bk_boolean expectingSequence = BK_FALSE;
    bk_boolean nextSequenceReady = BK_FALSE;
    struct bk_token_t currSubroutine;
    CLEAR(&currSubroutine);

    bk_boolean done = BK_FALSE;
    bk_result result = BK_SUCCESS;

    /* Read the event sequence. */
    while (!done) {
        /* Get the next event. */
        if (!yaml_parser_parse(&machine->parser, &event)) {
            if (machine->parser.error == YAML_PARSER_ERROR) {
                result = BK_PARSER_ERROR;
                goto error;
            }
            if (machine->parser.error == YAML_READER_ERROR || machine->parser.error == YAML_SCANNER_ERROR) {
                result = BK_IO_ERROR;
                goto error;
            }
            result = BK_FAILURE;
            goto error;
        }
        struct bk_token_t token;
        CLEAR(&token);

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
            nextSequenceReady = BK_TRUE;
            break;
        case YAML_MAPPING_END_EVENT:
            token.type = BK_TOKEN_IN_END;
            nextSequenceReady = BK_FALSE;
            break;
        case YAML_SEQUENCE_START_EVENT:
            token.type = BK_TOKEN_LIST_START;
            nextSequenceReady = BK_FALSE;
            break;
        case YAML_SEQUENCE_END_EVENT:
            token.type = BK_TOKEN_LIST_END;
            nextSequenceReady = BK_TRUE;
            break;

        case YAML_ALIAS_EVENT:
            break;
        case YAML_SCALAR_EVENT:
            if (expectingSequence) {
                snprintf(machine->lastErrorBuf, machine->lastErrorBufLen,
                    "Interpreter error on Line %zu-%zu Col %zu-%zu: Expected sequence but got scalar",
                    event.start_mark.line, event.end_mark.line, event.start_mark.column, event.end_mark.column);
                result = BK_INTERP_ERROR;
                goto error;
            }
            if (strcmp(event.data.scalar.value, BK_KEYWORD_IMPORT) == 0) {
                token.type = BK_TOKEN_DECL_IMPORT;
            }
            else if (strcmp(event.data.scalar.value, BK_KEYWORD_ARGS) == 0) {
                token.type = BK_TOKEN_DECL_ARGS;
            }
            else if (event.data.scalar.style == YAML_DOUBLE_QUOTED_SCALAR_STYLE || event.data.scalar.style == YAML_SINGLE_QUOTED_SCALAR_STYLE) {
                token.type = BK_TOKEN_USE_LIT;
                token.data.lit.value = event.data.scalar.value;
            }

            break;
        default:
            break;
        }

        /* Are we finished? */
        done = (event.type == YAML_STREAM_END_EVENT);

        yaml_event_delete(&event);
    }
    return BK_SUCCESS;

error:
    yaml_event_delete(&event);
    bk_internal_destroy_list(tokens);
    return result;
}

bk_result bk_interpret(bk_engine engine, const bk_unit pUnits, bk_integer numUnits) {

}

