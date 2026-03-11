#ifndef _ZB_BLINK_CORE_H_
#define _ZB_BLINK_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdio.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define BK_API_EXPORT  __declspec(dllexport)
#else
#define BK_API_EXPORT  
#endif

#if defined(blink_EXPORTS)
#define BK_API BK_API_EXPORT
#else
#if defined(_WIN32) && !defined(__MINGW32__)
#define BK_API  __declspec(dllimport)
#else
#define BK_API  
#endif
#endif

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
        BK_LIBRARY_FAILURE,

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
    typedef struct bk_managed_value_t* bk_managed_value;
    typedef struct bk_library_t* bk_library;

    typedef bk_result(*PFN_bk_engine_get_io)(bk_engine engine, FILE** pInput, FILE** pOutput);
    typedef void(*PFN_bk_engine_throw_exception)(bk_engine engine, bk_exception code, bk_string msg, bk_integer argc, bk_integer* argv);
    typedef bk_object(*PFN_bk_engine_create_object)(bk_engine engine, bk_metadata metadata, bk_voidptr data);
    typedef bk_voidptr(*PFN_bk_box_managed_value)(bk_voidptr value, bk_integer size, bk_type target);
    typedef bk_boolean(*PFN_bk_managed_value_coerce)(bk_managed_value value, bk_type target, bk_voidptr outData);
    typedef bk_type(*PFN_bk_managed_value_get_type)(bk_managed_value value);
    typedef bk_voidptr(*PFN_bk_managed_value_unbox)(bk_managed_value value, bk_type target);
    typedef bk_voidptr(*PFN_bk_managed_value_clone)(bk_managed_value value, bk_type target);
    typedef void(*PFN_bk_release_void_data)(bk_voidptr data);

    typedef struct bk_libctx_t {
        bk_engine engine;
        bk_unit unit;
        PFN_bk_engine_throw_exception engine_throw_exception;
        PFN_bk_engine_get_io engine_get_io;
        PFN_bk_engine_create_object engine_create_object;
        PFN_bk_managed_value_get_type managed_value_get_type;
        PFN_bk_managed_value_coerce managed_value_coerce;
        PFN_bk_box_managed_value box_managed_value;
        PFN_bk_managed_value_unbox managed_value_unbox;
        PFN_bk_managed_value_clone managed_value_clone;
        PFN_bk_release_void_data release_void_data;
    } bk_libctx;

    typedef bk_result(*bk_native_func)(bk_libctx* ctx, bk_integer argc, bk_managed_value* argv, bk_managed_value* pReturn);
    typedef bk_result(*PFN_bk_unknownlib_export)(bk_string* pLibName, bk_integer* pSubCount, bk_string** ppSubNames, bk_native_func** ppSubFptrs);
    typedef void(*PFN_bk_unknownlib_cleanup)();
 
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ZB_BLINK_H_