#ifndef _ZB_BLINK_CORE_H_
#define _ZB_BLINK_CORE_H_

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
    typedef void* bk_voidptr;
    typedef const char* bk_string;
    typedef struct bk_object_t* bk_object;

    typedef enum bk_result_t {
        BK_SUCCESS = 0,
        BK_FAILURE = -10000,

        BK_INIT_FAILURE,
        BK_NULL_FAILURE,

        BK_IO_ERROR,
        BK_PARSER_ERROR,
        BK_INTERP_ERROR,

        BK_ENGINE_EXCEPTION,
        BK_ENGINE_FAILURE,
        BK_ENGINE_NOT_IMPLEMENTED,
    } bk_result;

    typedef enum bk_type_t {
        BK_TYPE_NONE = 0,

        BK_TYPE_UNKNOWN = 'u',

        BK_TYPE_OBJECT = 'o',

        BK_TYPE_BOOLEAN = 'b',
        BK_TYPE_INTEGER = 'i',
        BK_TYPE_DECIMAL = 'd',

        BK_TYPE_STRING = 's',
    } bk_type;


    typedef enum bk_exception_t {
        BK_EX_NONE = 0,
        BK_EX_ENGINE_FAILURE = -0x7FFFFFFF,
        BK_EX = -0x0FFF,
        BK_EX_UNKNOWNSUB,
        BK_EX_UNKNOWNVAL,
        BK_EX_OBJECTCONVERSION,
        BK_EX_DIVISIONBYZERO,
        BK_EX_MISSINGARGUMENT,
        BK_EX_DOUBLEGATHER,
        BK_EX_NOTANUMBER,
        BK_EX_BADFORMAT,
        BK_EX_BADIO,
    } bk_exception;

    typedef struct bk_stream_t {
        FILE* pFile;
    } bk_stream;

    typedef struct bk_machine_t* bk_machine;
    typedef struct bk_engine_t* bk_engine;
    typedef struct bk_unit_t* bk_unit;
    typedef struct bk_metadata_t* bk_metadata;
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ZB_BLINK_H_