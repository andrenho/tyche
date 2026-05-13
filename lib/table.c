#include "priv.h"

#include "khash.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
KHASH_MAP_INIT_INT64(TABLE_INT, VALUE)
KHASH_MAP_INIT_STR(TABLE_STR, VALUE)
#pragma GCC diagnostic pop

struct Table {
    khash_t(TABLE_INT)* tbl_int;
    khash_t(TABLE_STR)* tbl_str;
    Heap const*         heap;
};

Table* table_new(Heap const* heap)
{
    Table* t = xcalloc(1, sizeof(Table));
    t->tbl_int = kh_init(TABLE_INT);
    t->tbl_str = kh_init(TABLE_STR);
    t->heap = heap;
    return t;
}

void table_destroy(Table* t)
{
    kh_destroy(TABLE_INT, t->tbl_int);
    kh_destroy(TABLE_STR, t->tbl_str);
    free(t);
}

size_t table_len(Table* t)
{
    return kh_size(t->tbl_int) + kh_size(t->tbl_str);
}

static TABLE_HASH value_hash(VALUE v)
{
    switch (value_type(v)) {
        case TT_NIL:
            return 0;
        case TT_INTEGER:
            return (uint64_t) value_integer(v);
        case TT_REAL: {
            uint32_t vv;
            float f = value_real(v);
            memcpy(&vv, &f, sizeof(uint32_t));
            break;
        }
        case TT_STRING_CONST:
            return (TABLE_HASH) value_idx(v) | ((TABLE_HASH) 1 << 33);
        case TT_ARRAY:
            return (TABLE_HASH) value_idx(v) | ((TABLE_HASH) 1 << 34);
        case TT_TABLE:
            return (TABLE_HASH) value_idx(v) | ((TABLE_HASH) 1 << 35);
        case TT_FUNCTION:
            return (TABLE_HASH) value_idx(v) | ((TABLE_HASH) 1 << 36);
        case TT_NATIVE_PTR:
            return (TABLE_HASH) value_idx(v) | ((TABLE_HASH) 1 << 37);
        case TT_STRING:
        default:
            __builtin_unreachable();
    }

    return 0;
}

TYC_RESULT table_get(Table const* t, VALUE key, VALUE* value)
{
    if (value_type(key) == TT_STRING) {
        const char* skey;
        if (heap_get_string(t->heap, value_idx(key), &skey) != T_OK)
            abort();
        khiter_t k = kh_get(TABLE_STR, t->tbl_str, skey);
        if (k == kh_end(t->tbl_str))
            return T_ERR_TABLE_KEY_NOT_FOUND;
        *value = kh_value(t->tbl_str, k);

    } else {
        TABLE_HASH hash = value_hash(key);
        khiter_t k = kh_get(TABLE_INT, t->tbl_int, hash);
        if (k == kh_end(t->tbl_int))
            return T_ERR_TABLE_KEY_NOT_FOUND;
        *value = kh_value(t->tbl_int, k);
    }

    return T_OK;
}

void table_set(Table* t, VALUE key, VALUE value)
{
    int ret;

    if (value_type(key) == TT_STRING) {
        const char* skey;
        if (heap_get_string(t->heap, value_idx(key), &skey) != T_OK)
            abort();
        khiter_t k = kh_put(TABLE_STR, t->tbl_str, skey, &ret);
        if (ret < 0)
            out_of_memory();
        kh_value(t->tbl_str, k) = value;

    } else {
        TABLE_HASH hash = value_hash(key);
        khiter_t k = kh_put(TABLE_INT, t->tbl_int, hash, &ret);
        if (ret < 0)
            out_of_memory();
        kh_value(t->tbl_int, k) = value;
    }
}

void table_del(Table* t, VALUE key)
{
    if (value_type(key) == TT_STRING) {
        const char* skey;
        if (heap_get_string(t->heap, value_idx(key), &skey) != T_OK)
            abort();
        khiter_t k = kh_get(TABLE_STR, t->tbl_str, skey);
        if (k == kh_end(t->tbl_str))
            return;
        kh_del(TABLE_STR, t->tbl_str, k);

    } else {
        TABLE_HASH hash = value_hash(key);
        khiter_t k = kh_get(TABLE_INT, t->tbl_int, hash);
        if (k == kh_end(t->tbl_int))
            return;
        kh_del(TABLE_INT, t->tbl_int, k);
    }
}
