#include "priv.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

TYC_TYPE value_type(VALUE v)
{
    return v.type;
}

const char* type_name(TYC_TYPE t)
{
    switch (t) {
        case TT_NIL:        return "nil";
        case TT_INTEGER:    return "integer";
        case TT_REAL:       return "real";
        case TT_STRING:     return "string";
        case TT_ARRAY:      return "array";
        case TT_TABLE:      return "table";
        case TT_FUNCTION:   return "function";
        case TT_NATIVE_PTR: return "native pointer";
        case TT_COUNT__:
        default:
            return "invalid type";
    }
}

bool type_is_collectable(TYC_TYPE t)
{
    return t == TT_STRING || t == TT_ARRAY || t == TT_TABLE;
}

int32_t value_integer(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (v.type != TT_INTEGER) {
        fprintf(stderr, "Expected integer, found %s.\n", type_name(v.type));
        abort();
    }
#endif
    return v.v.i;
}

float value_real(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (v.type != TT_REAL){
        fprintf(stderr, "Expected real, found %s.\n", type_name(v.type));
        abort();
    }
#endif
    return v.v.f;
}

uint32_t value_idx(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (v.type != TT_FUNCTION && v.type != TT_NATIVE_PTR){
        fprintf(stderr, "Expected function or native, found %s.\n", type_name(v.type));
        abort();
    }
#endif
    return v.v.idx;
}

HEAP_KEY value_heap_key(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (v.type != TT_ARRAY && v.type != TT_TABLE && v.type != TT_STRING){
        fprintf(stderr, "Expected table, array or string, found %s.\n", type_name(v.type));
        abort();
    }
#endif
    return v.v.idx;
}

VALUE create_value_nil(void)
{
    return (VALUE) { .type = TT_NIL };
}

VALUE create_value_from_bool(bool b)
{
    return b ? create_value_integer(1) : create_value_integer(0);
}

VALUE create_value_integer(int32_t v)
{
    return (VALUE) { .type = TT_INTEGER, .v = { .i = v } };
}

VALUE create_value_real(float f)
{
    return (VALUE) { .type = TT_REAL, .v = { .f = f } };
}

VALUE create_value_idx(TYC_TYPE type, uint32_t idx)
{
#ifdef CHECK_TYCHE_BUGS
    if (type != TT_FUNCTION && type != TT_NATIVE_PTR)
        abort();
#endif
    return (VALUE) { .type = type, .v = { .idx = idx } };
}

VALUE create_value_heap_key(TYC_TYPE type, HEAP_KEY key)
{
#ifdef CHECK_TYCHE_BUGS
    if (type != TT_ARRAY && type != TT_TABLE && type != TT_STRING)
        abort();
#endif
    return (VALUE) { .type = type, .v = { .heap_key = key } };
}

bool value_is_zero(VALUE v)
{
    return v.type == TT_NIL || (v.type == TT_INTEGER && v.v.i == 0);
}