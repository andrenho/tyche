#include "tyche.h"

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    TYC_TYPE type;
    union {
        int32_t  i;
        float    f;
        uint32_t idx;
    };
} VALUE;

static_assert(sizeof(VALUE) <= 8, "VALUE must be < 8 bytes");

static TYC_TYPE value_type(VALUE v)
{
    return v.type;
}

static int32_t value_integer(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (v.type != TT_INTEGER)
        abort();
#endif
    return v.i;
}

static float value_real(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (v.type != TT_REAL)
        abort();
#endif
    return v.f;
}

static uint32_t value_idx(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (v.type != TT_FUNCTION && v.type != TT_NATIVE_PTR && v.type != TT_ARRAY && v.type != TT_TABLE && v.type != TT_STRING && v.type != TT_STRING_CONST)
        abort();
#endif
    return v.idx;
}

static VALUE create_value_integer(int32_t v)
{
    return (VALUE) { .type = TT_INTEGER, .i = v };
}

static VALUE create_value_real(float f)
{
    return (VALUE) { .type = TT_INTEGER, .f = f };
}

static VALUE create_value_idx(TYC_TYPE type, uint32_t idx)
{
#ifdef CHECK_TYCHE_BUGS
    if (type != TT_FUNCTION && type != TT_NATIVE_PTR && type != TT_ARRAY && type != TT_TABLE && type != TT_STRING && type != TT_STRING_CONST)
        abort();
#endif
    return (VALUE) { .type = type, .idx = idx };
}