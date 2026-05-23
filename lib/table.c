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
    Table*              super;
};

Table* table_new(Heap const* heap)
{
    Table* t = xcalloc(1, sizeof(Table));
    t->tbl_int = kh_init(TABLE_INT);
    t->heap = heap;
    t->super = NULL;
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
    return v.as_int64;
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
        if (k == kh_end(t->tbl_int)) {                  // if not found,
            if (t->super)                               //   look into the supertable
                return table_get(t->super, key, value);
            else
                *value = create_value_nil();            //   no supertable, just return nil
        } else {
            *value = kh_value(t->tbl_int, k).value;     // return found record
        }
    }

    return T_OK;
}

bool table_has_key(Table const* t, VALUE key)
{
    TABLE_HASH hash = value_hash(key);
    khiter_t k = kh_get(TABLE_INT, t->tbl_int, hash);
    return k != kh_end(t->tbl_int);
}

void table_del(Table* t, VALUE key)
{
    TABLE_HASH hash = value_hash(key);
    khiter_t k = kh_get(TABLE_INT, t->tbl_int, hash);
    if (k == kh_end(t->tbl_int))
        return;
    kh_del(TABLE_INT, t->tbl_int, k);
}

static bool table_next_with_child(Table* t, Table* child, VALUE key, VALUE* out_key, VALUE* out_value)
{
    // if nil, start from the beginning, else find next
    khint_t k;
    if (value_type(key) == TT_NIL) {
        k = kh_begin(t->tbl_int);
    } else {
        TABLE_HASH hash = value_hash(key);
        k = kh_get(TABLE_INT, t->tbl_int, hash);
        ++k;
    }

skip_next:
    // skip non-existing records
    while (kh_size(t->tbl_int) > 0 && !kh_exist(t->tbl_int, k) && k < kh_end(t->tbl_int))
        ++k;

    // if not found, return nil, otherwise return the record
    if (kh_size(t->tbl_int) == 0 || k >= kh_end(t->tbl_int)) {
        if (t->super) {
            return table_next_with_child(t->super, t, key, out_key, out_value);
        } else {
            if (out_key) *out_key = create_value_nil();
            if (out_value) *out_value = create_value_nil();
            return false;
        }
    } else {
        // before returning, check if key exists in child
        //   if it does, we should skip it, as it was already returned
        if (child && table_has_key(child, kh_value(t->tbl_int, k).key)) {
            ++k;
            goto skip_next;
        }

        // return found records
        if (out_key) *out_key = kh_value(t->tbl_int, k).key;
        if (out_value) *out_value = kh_value(t->tbl_int, k).value;
        return true;
    }
}

bool table_next(Table* t, VALUE key, VALUE* out_key, VALUE* out_value)
{
    return table_next_with_child(t, NULL, key, out_key, out_value);
}

size_t table_size(Table const* t)
{
    return kh_size(t->tbl_int);
}

void table_setsuper(Table* t, Table* super)
{
    t->super = super;

    if (super != NULL) {
        VALUE key = create_value_nil(), value;
        while (table_next(t->super, key, &key, &value)) {
            if (value_type(value) != TT_FUNCTION)
                table_set(t, key, value);
        }
    }
}
