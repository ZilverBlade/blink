#ifndef _ZB_BLINK_H_
#define _ZB_BLINK_H_

#include "blink_core.h"
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdio.h>

    BK_API bk_result bk_create_interpreter(bk_machine* pResult);
    BK_API void bk_destroy_interpreter(bk_machine machine);

    BK_API bk_string bk_interpreter_get_error(bk_machine machine);

    BK_API bk_result bk_create_execution_engine(bk_engine* pResult);
    BK_API void bk_destroy_execution_engine(bk_engine engine);

    BK_API bk_result bk_engine_set_io(bk_engine engine, FILE* input, FILE* output);
    BK_API bk_result bk_engine_get_io(bk_engine engine, FILE** pInput, FILE** pOutput);
    BK_API bk_result bk_engine_reset_state(bk_engine engine);
    BK_API void bk_engine_throw_exception(bk_engine engine, bk_exception code, bk_string msg, bk_integer argc, bk_integer* argv);
    BK_API bk_result bk_engine_create_object(bk_engine engine, bk_metadata metadata, bk_voidptr data, bk_object* pResult);
    BK_API bk_string bk_engine_get_error(bk_engine engine);
    BK_API bk_result bk_engine_attach_library(bk_engine engine, bk_voidptr library);

    BK_API bk_result bk_compile_translation_unit(bk_machine machine,  bk_stream* pStream, bk_string name, bk_unit* pResult);
    BK_API bk_result bk_emit_translation_unit(bk_unit unit, char* outBuf, bk_integer* pBufLen);
    BK_API void bk_release_translation_unit(bk_unit unit);

    BK_API bk_result bk_execute(bk_engine engine, bk_unit* pUnits, bk_integer numUnits, bk_integer entrypoint);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ZB_BLINK_H_