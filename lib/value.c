#include "priv.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    TTT_STRING    = NANBOX_MIN_AUX_TAG,
    TTT_ARRAY     = NANBOX_MIN_AUX_TAG + 1,
    TTT_TABLE     = NANBOX_MIN_AUX_TAG + 2,
    TTT_FUNCTION  = NANBOX_MIN_AUX_TAG + 3,
    TTT_NATIVE_FN = NANBOX_MIN_AUX_TAG + 4,
} TagType;

TYC_TYPE value_type(VALUE v)
{
    if (nanbox_is_null(v))
        return TYC_NIL;
    if (nanbox_is_boolean(v))
        return TYC_BOOLEAN;
    if (nanbox_is_int(v))
        return TYC_INTEGER;
    if (nanbox_is_double(v))
        return TYC_REAL;
    if (nanbox_is_pointer(v))
        return TYC_NATIVE_PTR;
    switch (v.as_bits.tag) {
        case TTT_STRING:    return TYC_STRING;
        case TTT_ARRAY:     return TYC_ARRAY;
        case TTT_TABLE:     return TYC_TABLE;
        case TTT_FUNCTION:  return TYC_FUNCTION;
        case TTT_NATIVE_FN: return TYC_NATIVE_FN__;
        default:;
    }

    fprintf(stderr, "Undefined value type\n");
    abort();
}

const char* type_name(TYC_TYPE t)
{
    switch (t) {
        case TYC_NIL:        return "nil";
        case TYC_BOOLEAN:    return "boolean";
        case TYC_INTEGER:    return "integer";
        case TYC_REAL:       return "real";
        case TYC_STRING:     return "string";
        case TYC_ARRAY:      return "array";
        case TYC_TABLE:      return "table";
        case TYC_FUNCTION:   return "function";
        case TYC_NATIVE_PTR: return "native pointer";
        case TYC_NATIVE_FN__:  return "native function";
        case TYC_COUNT__:
        default:
            return "invalid type";
    }
}

bool type_is_collectable(TYC_TYPE t)
{
    return t == TYC_STRING || t == TYC_ARRAY || t == TYC_TABLE;
}

bool value_boolean(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TYC_BOOLEAN) {
        fprintf(stderr, "Expected boolean, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    return nanbox_to_boolean(v);
}

int32_t value_integer(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TYC_INTEGER && value_type(v) != TYC_REAL) {
        fprintf(stderr, "Expected number, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    if (nanbox_is_double(v))
        return (int) nanbox_to_double(v);

    return nanbox_to_int(v);
}

TYCHE_REAL value_real(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TYC_INTEGER && value_type(v) != TYC_REAL){
        fprintf(stderr, "Expected number, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    if (nanbox_is_int(v))
        return (TYCHE_REAL) nanbox_to_int(v);
    return nanbox_to_double(v);
}

uint32_t value_function_idx(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TYC_FUNCTION) {
        fprintf(stderr, "Expected function, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    return v.as_bits.payload;
}

HEAP_KEY value_heap_key(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TYC_ARRAY && value_type(v) != TYC_TABLE && value_type(v) != TYC_STRING && value_type(v) != TYC_NATIVE_FN__) {
        fprintf(stderr, "Expected table, array or string, found %s.\n", type_name(value_type(v)));
        abort();
    }
#endif
    return v.as_bits.payload;
}

void* value_native_pointer(VALUE v)
{
#ifdef CHECK_TYCHE_BUGS
    if (value_type(v) != TYC_NATIVE_PTR) {
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

VALUE create_value_bool(bool b)
{
    return nanbox_from_boolean(b);
}

VALUE create_value_integer(int32_t v)
{
    return nanbox_from_int(v);
}

VALUE create_value_real(TYCHE_REAL f)
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
    if (type != TYC_ARRAY && type != TYC_TABLE && type != TYC_STRING && type != TYC_NATIVE_FN__)
        abort();
#endif
    if (type == TYC_ARRAY)
        return (VALUE) { .as_bits = { .tag = TTT_ARRAY, .payload = key } };
    if (type == TYC_TABLE)
        return (VALUE) { .as_bits = { .tag = TTT_TABLE, .payload = key } };
    if (type == TYC_STRING)
        return (VALUE) { .as_bits = { .tag = TTT_STRING, .payload = key } };
    if (type == TYC_NATIVE_FN__)
        return (VALUE) { .as_bits = { .tag = TTT_NATIVE_FN, .payload = key } };
    __builtin_unreachable();
}

VALUE create_value_native_pointer(void* ptr)
{
    return nanbox_from_pointer(ptr);
}

bool value_is_zero(VALUE v)
{
    return value_type(v) == TYC_NIL || (value_type(v) == TYC_INTEGER && value_integer(v) == 0);
}