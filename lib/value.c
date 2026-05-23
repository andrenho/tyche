#include "priv.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    TTT_STRING   = NANBOX_MIN_AUX_TAG,
    TTT_ARRAY    = NANBOX_MIN_AUX_TAG + 1,
    TTT_TABLE    = NANBOX_MIN_AUX_TAG + 2,
    TTT_FUNCTION = NANBOX_MIN_AUX_TAG + 3,
} TagType;

TYC_TYPE value_type(VALUE v)
{
    if (nanbox_is_null(v))
        return TT_NIL;
    if (nanbox_is_int(v))
        return TT_INTEGER;
    if (nanbox_is_double(v))
        return TT_REAL;
    if (nanbox_is_pointer(v))
        return TT_NATIVE_PTR;
    switch (v.as_bits.tag) {
        case TTT_STRING:   return TT_STRING;
        case TTT_ARRAY:    return TT_ARRAY;
        case TTT_TABLE:    return TT_TABLE;
        case TTT_FUNCTION: return TT_FUNCTION;
        default:;
    }

    fprintf(stderr, "Undefined value type\n");
    abort();
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
    if (value_type(v) != TT_INTEGER) {
        fprintf(stderr, "Expected integer, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    return nanbox_to_int(v);
}

T_REAL value_real(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TT_REAL){
        fprintf(stderr, "Expected real, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    return nanbox_to_double(v);
}

uint32_t value_function_idx(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TT_FUNCTION) {
        fprintf(stderr, "Expected function, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    return v.as_bits.payload;
}

HEAP_KEY value_heap_key(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TT_ARRAY && value_type(v) != TT_TABLE && value_type(v) != TT_STRING){
        fprintf(stderr, "Expected table, array or string, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    return v.as_bits.payload;
}

void* value_native_pointer(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TT_NATIVE_PTR) {
        fprintf(stderr, "Expected native pointer, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    return nanbox_to_pointer(v);
}

VALUE create_value_nil(void)
{
    return nanbox_null();
}

VALUE create_value_from_bool(bool b)
{
    return b ? create_value_integer(1) : create_value_integer(0);
}

VALUE create_value_integer(int32_t v)
{
    return nanbox_from_int(v);
}

VALUE create_value_real(T_REAL f)
{
    return nanbox_from_double(f);
}

VALUE create_value_function_idx(uint32_t idx)
{
    return (VALUE) { .as_bits = { .tag = TTT_FUNCTION, .payload = idx } };
}

VALUE create_value_heap_key(TYC_TYPE type, HEAP_KEY key)
{
#ifdef CHECK_TYCHE_BUGS
    if (type != TT_ARRAY && type != TT_TABLE && type != TT_STRING)
        abort();
#endif
    if (type == TT_ARRAY)
        return (VALUE) { .as_bits = { .tag = TTT_ARRAY, .payload = key } };
    if (type == TT_TABLE)
        return (VALUE) { .as_bits = { .tag = TTT_TABLE, .payload = key } };
    return (VALUE) { .as_bits = { .tag = TTT_STRING, .payload = key } };
}

VALUE create_value_native_pointer(void* ptr)
{
    return nanbox_from_pointer(ptr);
}

bool value_is_zero(VALUE v)
{
    return value_type(v) == TT_NIL || (value_type(v) == TT_INTEGER && value_integer(v) == 0);
}