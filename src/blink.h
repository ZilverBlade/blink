#ifndef _ZB_BLINK_H_
#define _ZB_BLINK_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdio.h>

#define BK_API

    typedef int bk_boolean;
#define BK_TRUE 1
#define BK_FALSE 0

    typedef int32_t bk_integer;
    typedef float bk_decimal;
    typedef void* bk_object;
    typedef const char* bk_string;

    typedef enum bk_result_t {
        BK_SUCCESS = 0,
        BK_FAILURE = -10000,

        BK_INIT_FAILURE,
        BK_NULL_FAILURE,

        BK_IO_ERROR,
        BK_PARSER_ERROR,
        BK_INTERP_ERROR,

        BK_EXEC_EXCEPTION,
        BK_EXEC_FAILURE,
    } bk_result;

    typedef enum bk_type_t {
        BK_TYPE_UNKNOWN,

        BK_TYPE_OBJECT,

        BK_TYPE_BOOLEAN,
        BK_TYPE_INTEGER,
        BK_TYPE_DECIMAL,

        BK_TYPE_STRING,
    } bk_type;

    typedef struct bk_stream_t {
        FILE* pFile;
    } bk_stream;

    typedef struct bk_machine_t* bk_machine;
    typedef struct bk_engine_t* bk_engine;
    typedef struct bk_unit_t* bk_unit;

    BK_API bk_result bk_create_interpreter(bk_machine* pResult);
    BK_API void bk_destroy_interpreter(bk_machine machine);

    BK_API bk_string bk_get_interpreter_error(bk_machine machine);

    BK_API bk_result bk_create_execution_engine(bk_engine* pResult);
    BK_API void bk_destroy_execution_engine(bk_engine engine);

    BK_API bk_result bk_set_execution_engine_io(bk_engine engine, FILE* input, FILE* output);
    BK_API bk_result bk_reset_execution_engine_state(bk_engine engine);

    BK_API bk_result bk_compile_translation_unit(bk_machine machine, bk_stream* pStream, bk_unit* pResult);
    BK_API bk_result bk_emit_translation_unit(bk_unit unit, char* outBuf, bk_integer* pBufLen);
    BK_API void bk_release_translation_unit(bk_unit unit);

    BK_API bk_result bk_execute(bk_engine engine, bk_unit* pUnits, bk_integer numUnits);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ZB_BLINK_H_