#include "../blink_core.h"

#include <math.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// --- Helpers ---
static int IsNotADecimal(bk_decimal num) {
    return isnan(num) || isinf(num);
}

static bk_result get_decimal_arg(bk_libctx* ctx, bk_managed_value val, bk_decimal* out_val, bk_integer arg_idx) {
    if (!ctx->managed_value_coerce(val, BK_TYPE_DECIMAL, out_val)) {
        ctx->engine_throw_exception(ctx->engine, BK_EX_OBJECTCONVERSION, "Failed to coerce argument", 1, &arg_idx);
        return BK_ENGINE_EXCEPTION;
    }

    if (IsNotADecimal(*out_val)) {
        ctx->engine_throw_exception(ctx->engine, BK_EX_NOTANUMBER, "Argument is not a number", 1, &arg_idx);
        return BK_ENGINE_EXCEPTION;
    }
    return BK_SUCCESS;
}

#define CHECK_ARGC(expected) \
    if (argc != (expected)) { \
        bk_integer arg = expected; \
        ctx->engine_throw_exception(ctx->engine, BK_EX_MISSINGARGUMENT, "Invalid number of arguments", 1, &arg); \
        return BK_ENGINE_EXCEPTION; \
    }

// --- Native Math Library ---

static bk_result bkstdlib_add(bk_libctx* ctx, bk_integer argc, bk_managed_value* argv, bk_managed_value* pReturn) {
    CHECK_ARGC(2);
    bk_decimal a, b;
    if (get_decimal_arg(ctx, argv[0], &a, 0) != BK_SUCCESS) return BK_ENGINE_EXCEPTION;
    if (get_decimal_arg(ctx, argv[1], &b, 1) != BK_SUCCESS) return BK_ENGINE_EXCEPTION;

    bk_decimal result = a + b;
    *pReturn = ctx->box_managed_value(&result, sizeof(bk_decimal), BK_TYPE_DECIMAL);
    return BK_SUCCESS;
}

static bk_result bkstdlib_sub(bk_libctx* ctx, bk_integer argc, bk_managed_value* argv, bk_managed_value* pReturn) {
    CHECK_ARGC(2);
    bk_decimal a, b;
    if (get_decimal_arg(ctx, argv[0], &a, 0) != BK_SUCCESS) return BK_ENGINE_EXCEPTION;
    if (get_decimal_arg(ctx, argv[1], &b, 1) != BK_SUCCESS) return BK_ENGINE_EXCEPTION;

    bk_decimal result = a - b;
    *pReturn = ctx->box_managed_value(&result, sizeof(bk_decimal), BK_TYPE_DECIMAL);
    return BK_SUCCESS;
}

static bk_result bkstdlib_mul(bk_libctx* ctx, bk_integer argc, bk_managed_value* argv, bk_managed_value* pReturn) {
    CHECK_ARGC(2);
    bk_decimal a, b;
    if (get_decimal_arg(ctx, argv[0], &a, 0) != BK_SUCCESS) return BK_ENGINE_EXCEPTION;
    if (get_decimal_arg(ctx, argv[1], &b, 1) != BK_SUCCESS) return BK_ENGINE_EXCEPTION;


    bk_decimal result = a * b;
    *pReturn = ctx->box_managed_value(&result, sizeof(bk_decimal), BK_TYPE_DECIMAL);
    return BK_SUCCESS;
}

static bk_result bkstdlib_div(bk_libctx* ctx, bk_integer argc, bk_managed_value* argv, bk_managed_value* pReturn) {
    CHECK_ARGC(2);
    bk_decimal a, b;
    if (get_decimal_arg(ctx, argv[0], &a, 0) != BK_SUCCESS) return BK_ENGINE_EXCEPTION;
    if (get_decimal_arg(ctx, argv[1], &b, 1) != BK_SUCCESS) return BK_ENGINE_EXCEPTION;

    if (b * b < 1e-12f) {
        bk_integer arg = 1;
        ctx->engine_throw_exception(ctx->engine, BK_EX_DIVISIONBYZERO, "Division by zero!", 1, &arg);
        return BK_ENGINE_EXCEPTION;
    }

    bk_decimal result = a / b;
    *pReturn = ctx->box_managed_value(&result, sizeof(bk_decimal), BK_TYPE_DECIMAL);
    return BK_SUCCESS;
}


static bk_result bkstdlib_cat(bk_libctx* ctx, bk_integer argc, bk_managed_value* argv, bk_managed_value* pReturn) {
    CHECK_ARGC(2);
    *pReturn = ctx->box_managed_value(NULL, 0, BK_TYPE_UNKNOWN);
    return BK_SUCCESS;
}


// --- Native IO Library ---

static bk_result bkstdlib_writeout(bk_libctx* ctx, bk_integer argc, bk_managed_value* argv, bk_managed_value* pReturn) {
    FILE* out;

    // Fixed: Use ctx->engine_get_io instead of undefined bk_engine_get_io
    if (ctx->engine_get_io(ctx->engine, NULL, &out) != BK_SUCCESS) {
        ctx->engine_throw_exception(ctx->engine, BK_EX_ENGINE_FAILURE, "Failed to get IO!", 0, NULL);
        return BK_ENGINE_EXCEPTION;
    }
    if (!out) {
        ctx->engine_throw_exception(ctx->engine, BK_EX_BADIO, "Missing output FILE", 0, NULL);
        return BK_ENGINE_EXCEPTION;
    }

    if (argc > 0) {
        bk_string coercestr = NULL;

        if (ctx->managed_value_coerce(argv[0], BK_TYPE_STRING, &coercestr) == BK_SUCCESS) {
            bk_string fmt_str = coercestr;
            bk_integer argi = 1;
            while (*fmt_str) {
                int c = *fmt_str;
                if (c == '$') {
                    ++fmt_str;
                    switch (*fmt_str) {
                    case '$':
                        putc('$', out);
                        continue;
                    default: {
                        CHECK_ARGC(argi + 1);
                        bk_string str;
                        if (ctx->managed_value_coerce(argv[argi], BK_TYPE_STRING, &str) != BK_SUCCESS) {
                            ctx->engine_throw_exception(ctx->engine, BK_EX_OBJECTCONVERSION, "Failed to coerce argument", 1, &argi);
                            return BK_ENGINE_EXCEPTION;
                        }
                        ++argi;
                        fprintf(out, "%s", str);
                        continue;
                    }
                    }
                }
                putc(c, out);
                ++fmt_str;
            }
        }
    }
    else {
        fprintf(out, "\n");
    }

    *pReturn = ctx->box_managed_value(NULL, 0, BK_TYPE_UNKNOWN);
    return BK_SUCCESS;
}

// --- Export Definitions ---

static struct subs_t {
    bk_string szName;
    bk_native_func pfnSub;
} subs[] = {
    {.szName = "add",      .pfnSub = bkstdlib_add,     },
    {.szName = "sub",      .pfnSub = bkstdlib_sub,     },
    {.szName = "mul",      .pfnSub = bkstdlib_mul,     },
    {.szName = "div",      .pfnSub = bkstdlib_div,     },
    {.szName = "cat",      .pfnSub = bkstdlib_cat,     },
    {.szName = "writeout", .pfnSub = bkstdlib_writeout },
};

static bk_string* s_pSubNames = NULL;
static bk_string* s_pSubFptrs = NULL;

BK_API_EXPORT bk_result bk_unknownlib_export(
    bk_string* pLibName, bk_integer* pSubCount,
    bk_string** ppSubNames, bk_native_func** ppSubFptrs) {

    *pLibName = "std"; // Provide the library name
    *pSubCount = sizeof(subs) / sizeof(struct subs_t);

    s_pSubNames = malloc(sizeof(bk_string) * *pSubCount);
    s_pSubFptrs = malloc(sizeof(bk_native_func) * *pSubCount);

    *ppSubNames = s_pSubNames;
    *ppSubFptrs = s_pSubFptrs;

    for (bk_integer i = 0; i < *pSubCount; ++i) {
        (*ppSubNames)[i] = subs[i].szName;
        (*ppSubFptrs)[i] = subs[i].pfnSub;
    }
    return BK_SUCCESS;
}
BK_API_EXPORT void bk_unknownlib_cleanup() {
    free(s_pSubNames);
    free(s_pSubFptrs);
}