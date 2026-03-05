#include "../blink_core.h"
#include "../blink.h"

#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define BK_API_EXPORT  __declspec(dllexport)
#else
#define BK_API_EXPORT  
#endif

static int IsNotADecimal(bk_decimal num) {
    return isnan(num) || isinf(num);
}

static bk_decimal bkstdlib_add(bk_engine engine, bk_decimal a, bk_decimal b) {
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
static bk_decimal bkstdlib_sub(bk_engine engine, bk_decimal a, bk_decimal b) {
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
static bk_decimal bkstdlib_mul(bk_engine engine, bk_decimal a, bk_decimal b) {
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
static bk_decimal bkstdlib_div(bk_engine engine, bk_decimal a, bk_decimal b) {
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

static void bkstdlib_writeout(bk_engine engine, bk_string fmt, bk_integer argc, bk_voidptr* argv) {
    FILE* out;
    if (bk_engine_get_io(engine, NULL, &out) != BK_SUCCESS) {
        bk_engine_throw_exception(engine, BK_EX_ENGINE_FAILURE, "Failed to get IO!", 0, NULL);
        return;
    }
    if (!out) {
        bk_engine_throw_exception(engine, BK_EX_BADIO, "Missing output FILE", 0, NULL);
        return;
    };
    if (argc == 0) {
        fprintf(out, "%s\n", fmt);
    }
}

static struct subs_t {
    bk_string szName;
    bk_voidptr pfnSub;
    bk_string szSig;
} subs[] = {
    {.szName = "add", .pfnSub = bkstdlib_add, .szSig = "ddd" },
    {.szName = "sub", .pfnSub = bkstdlib_sub, .szSig = "ddd"  },
    {.szName = "add", .pfnSub = bkstdlib_mul, .szSig = "ddd"  },
    {.szName = "add", .pfnSub = bkstdlib_div, .szSig = "ddd"  },
    {.szName = "writeout", .pfnSub = bkstdlib_writeout, .szSig = "us..."  },
};

static bk_result bkunknownlib_export(
    bk_string* pLibName, bk_integer* pSubCount,
    bk_string** ppSubNames, bk_voidptr** ppSubFptrs, bk_string** ppSubSigs) {
    *pSubCount = sizeof(subs) / sizeof(struct subs_t);
    *ppSubNames = malloc(sizeof(bk_string*) * *pSubCount);
    *ppSubFptrs = malloc(sizeof(bk_voidptr*) * *pSubCount);
    *ppSubSigs = malloc(sizeof(bk_string*) * *pSubCount);
    for (bk_integer i = 0; i < *pSubCount; ++i) {
        (*ppSubNames)[i] = subs[i].szName;
        (*ppSubFptrs)[i] = subs[i].pfnSub;
        (*ppSubSigs)[i] = subs[i].szSig;
    }
    return BK_SUCCESS;
}