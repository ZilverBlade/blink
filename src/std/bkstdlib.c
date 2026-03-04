#include "../blink.h"

#include <math.h>
#include <stdarg.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define BK_API_EXPORT  __declspec(dllexport)
#else
#define BK_API_EXPORT  
#endif

static int IsNotADecimal(bk_decimal num) {
    return isnan(num) || isinf(num);
}

BK_API_EXPORT bk_decimal bkstdlib_add(bk_engine engine, bk_decimal a, bk_decimal b) {
    if (IsNotADecimal(a)) {
        bk_integer arg = 0;
        bk_engine_throw_exception(engine, BK_EX_NOTANUMBER, "Not a number", 1, &arg);
        return 0.0;
    }
    if (IsNotADecimal(b)) {
        bk_integer arg = 1;
        bk_engine_throw_exception(engine, BK_EX_NOTANUMBER, "Not a number", 1, &arg);
        return 0.0;
    }
    return a + b;
}
BK_API_EXPORT bk_decimal bkstdlib_std_sub(bk_engine engine, bk_decimal a, bk_decimal b) {
    if (IsNotADecimal(a)) {
        bk_integer arg = 0;
        bk_engine_throw_exception(engine, BK_EX_NOTANUMBER, "Not a number", 1, &arg);
        return 0.0;
    }
    if (IsNotADecimal(b)) {
        bk_integer arg = 1;
        bk_engine_throw_exception(engine, BK_EX_NOTANUMBER, "Not a number", 1, &arg);
        return 0.0;
    }
    return a - b;
}
BK_API_EXPORT bk_decimal bkstdlib_std_mul(bk_engine engine, bk_decimal a, bk_decimal b) {
    if (IsNotADecimal(a)) {
        bk_integer arg = 0;
        bk_engine_throw_exception(engine, BK_EX_NOTANUMBER, "Not a number", 1, &arg);
        return 0.0;
    }
    if (IsNotADecimal(b)) {
        bk_integer arg = 1;
        bk_engine_throw_exception(engine, BK_EX_NOTANUMBER, "Not a number", 1, &arg);
        return 0.0;
    }
    return a * b;
}
BK_API_EXPORT bk_decimal bkstdlib_std_div(bk_engine engine, bk_decimal a, bk_decimal b) {
    if (IsNotADecimal(a)) {
        bk_integer arg = 0;
        bk_engine_throw_exception(engine, BK_EX_NOTANUMBER, NULL, 1, &arg);
        return 0.0;
    }
    if (IsNotADecimal(b)) {
        bk_integer arg = 1;
        bk_engine_throw_exception(engine, BK_EX_NOTANUMBER, NULL, 1, &arg);
        return 0.0;
    }
    if (b == 0.0f) {
        bk_integer arg = 1;
        bk_engine_throw_exception(engine, BK_EX_DIVISIONBYZERO, NULL, 1, &arg);
        return 0.0;
    }
    return a / b;
}

BK_API_EXPORT void bkstdlib_writeout(bk_engine engine, bk_string fmt, ...) {
    FILE* out;
    if (bk_engine_get_io(engine, NULL, &out) != BK_SUCCESS) {
        bk_engine_throw_exception(engine, BK_EX_ENGINE_FAILURE, "Failed to get IO!", 0, NULL);
        return;
    }
    if (!out) {
        bk_engine_throw_exception(engine, BK_EX_BADIO, "Missing output FILE", 0, NULL);
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);
}