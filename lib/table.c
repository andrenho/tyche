#include "priv.h"

#include "khash.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
KHASH_MAP_INIT_INT64(TABLE_INT, VALUE)
KHASH_MAP_INIT_STR(TABLE_STR, VALUE)
#pragma GCC diagnostic pop

struct Table {
    khash_t(TABLE_INT) *tbl_int;
    khash_t(TABLE_STR) *tbl_str;
};

Table* table_new(void)
{
    Table* t = xcalloc(1, sizeof(Table));
    t->tbl_int = kh_init(TABLE_INT);
    t->tbl_str = kh_init(TABLE_STR);
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

static int64_t value_hash(VALUE v)
{
    switch (value_type(v)) {
        case TT_NIL:
            return 0;
        case TT_INTEGER:
            return value_integer(v);
        case TT_REAL: {
            uint32_t vv;
            float f = value_real(v);
            memcpy(&vv, &f, sizeof(uint32_t));
            break;
        }
        case TT_STRING_CONST:
            return (int64_t) value_idx(v) | ((int64_t) 1 << 33);
        case TT_ARRAY:
            return (int64_t) value_idx(v) | ((int64_t) 1 << 34);
        case TT_TABLE:
            return (int64_t) value_idx(v) | ((int64_t) 1 << 35);
        case TT_FUNCTION:
            return (int64_t) value_idx(v) | ((int64_t) 1 << 36);
        case TT_NATIVE_PTR:
            return (int64_t) value_idx(v) | ((int64_t) 1 << 37);
        case TT_STRING:
        default:
            __builtin_unreachable();
    }

    return 0;
}

VALUE table_get(Table const* t, VALUE key)
{
}

void table_set(Table* t, VALUE key, VALUE value)
{
    /*
    if (value_type(key) == TT_STRING) {
        khiter_t k = kh_get(TABLE_STR, t->tbl_str, value_);

    } else {
    }
     */
}
