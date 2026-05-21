#include "priv.h"

#include "khash.h"

typedef struct {
    VALUE key;
    VALUE value;
} TableValue;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
KHASH_MAP_INIT_INT64(TABLE_INT, TableValue)
#pragma GCC diagnostic pop

struct Table {
    khash_t(TABLE_INT)* tbl_int;
    Heap const*         heap;
};

Table* table_new(Heap const* heap)
{
    Table* t = xcalloc(1, sizeof(Table));
    t->tbl_int = kh_init(TABLE_INT);
    t->heap = heap;
    return t;
}

void table_destroy(Table* t)
{
    kh_destroy(TABLE_INT, t->tbl_int);
    free(t);
}

size_t table_len(Table* t)
{
    return kh_size(t->tbl_int);
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
        case TT_ARRAY:
        case TT_TABLE:
        case TT_STRING:
            return value_heap_key(v);
        case TT_FUNCTION:
            return (TABLE_HASH) value_idx(v) | ((TABLE_HASH) 1 << 36);
        case TT_NATIVE_PTR:
            return (TABLE_HASH) value_idx(v) | ((TABLE_HASH) 1 << 37);
        case TT_COUNT__:
        default:
            __builtin_unreachable();
    }

    return 0;
}

void table_set(Table* t, VALUE key, VALUE value)
{
    if (value_type(value) == TT_NIL) {  // if value = nil, delete from table
        table_del(t, key);
        return;
    }

    TABLE_HASH hash = value_hash(key);

    int ret;
    khiter_t k = kh_put(TABLE_INT, t->tbl_int, hash, &ret);
    if (ret < 0)
        out_of_memory();
    kh_value(t->tbl_int, k) = (TableValue) { key, value };
}

TYC_RESULT table_get(Table const* t, VALUE key, VALUE* value)
{
    TABLE_HASH hash = value_hash(key);
    khiter_t k = kh_get(TABLE_INT, t->tbl_int, hash);
    if (value) {
        if (k == kh_end(t->tbl_int))
            *value = create_value_nil();
        else
            *value = kh_value(t->tbl_int, k).value;
    }

    return T_OK;
}

void table_del(Table* t, VALUE key)
{
    TABLE_HASH hash = value_hash(key);
    khiter_t k = kh_get(TABLE_INT, t->tbl_int, hash);
    if (k == kh_end(t->tbl_int))
        return;
    kh_del(TABLE_INT, t->tbl_int, k);
}

void table_foreach(Table* t, void(f)(VALUE key, VALUE value, void* data), void* data)
{
    for (khiter_t k = kh_begin(t->tbl_int); k != kh_end(t->tbl_int); ++k)
        if (kh_exist(t->tbl_int, k))
            f(kh_value(t->tbl_int, k).key, kh_value(t->tbl_int, k).value, data);
}

bool table_next(Table* t, VALUE key, VALUE* out_key, VALUE* out_value)
{
    khint_t k;
    if (value_type(key) == TT_NIL) {
        k = kh_begin(t->tbl_int);
    } else {
        TABLE_HASH hash = value_hash(key);
        k = kh_get(TABLE_INT, t->tbl_int, hash);
        ++k;
    }

    if (kh_size(t->tbl_int) > 0)
        while (!kh_exist(t->tbl_int, k) && k != kh_end(t->tbl_int))
            ++k;

    if (k == kh_end(t->tbl_int)) {
        if (out_key) *out_key = create_value_nil();
        if (out_value) *out_value = create_value_nil();
        return false;
    } else {
        if (out_key) *out_key = kh_value(t->tbl_int, k).key;
        if (out_value) *out_value = kh_value(t->tbl_int, k).value;
        return true;
    }
}

size_t table_size(Table* t)
{
    return kh_size(t->tbl_int);
}
