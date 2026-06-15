#include "priv.h"

typedef struct {
    VALUE key;
    VALUE value;
} TableKV;

struct Table {
    size_t      sz;
    size_t      in_use;
    TableKV*    items;
    Table*      super;
    TycheVM const* T;
};

Table* table_new(TycheVM const* T)
{
    Table* t = xcalloc(1, sizeof(Table));
    t->sz = 8;
    t->in_use = 0;
    t->items = xmalloc(t->sz * sizeof(TableKV));
    for (size_t i = 0; i < t->sz; ++i)
        t->items[i].key = create_value_nil();
    t->T = T;
    t->super = NULL;
    return t;
}

void table_destroy(Table* t)
{
    free(t->items);
    free(t);
}

size_t table_len(Table const* t)
{
    size_t sz = t->in_use;
    if (t->super)
        sz += table_len(t->super);
    return sz;
}

static void table_rehash(Table* t)
{
    // copy items to a temporary table
    TableKV* items = xmalloc(t->in_use * sizeof(TableKV));
    size_t n_real_items = 0;
    for (size_t i = 0; i < t->sz; ++i)
        if (!value_is_nil(t->items[i].key) && !value_is_tombstone(t->items[i].key))
            items[n_real_items++] = t->items[i];

    // clear and resize table
    free(t->items);
    t->sz *= 2;
    t->in_use = 0;
    t->items = xmalloc(t->sz * sizeof(TableKV));
    for (size_t i = 0; i < t->sz; ++i)
        t->items[i].key = create_value_nil();

    // recreate table
    for (size_t i = 0; i < n_real_items; ++i)
        table_set(t, items[i].key, items[i].value);

    free(items);
}

void table_set(Table* t, VALUE key, VALUE value)
{
    if (value_is_nil(value)) {
        table_del(t, key);
        return;
    }

    if ((double) t->in_use / (double) t->sz > 0.7)
        table_rehash(t);

    uint32_t hash = tyc_hash(t->T, key);
    uint32_t idx = hash % t->sz;

    // loop from index to end
    for (size_t i = idx; i < t->sz; ++i) {
        if (value_is_nil(t->items[i].key)) {
            t->items[i] = (TableKV) { key, value };
            ++t->in_use;
            return;
        }
    }

    // loop from 0 to index
    if (idx > 0) {
        for (size_t i = 0; i < (idx-1); ++i) {
            if (value_is_nil(t->items[i].key)) {
                t->items[i] = (TableKV) { key, value };
                ++t->in_use;
                return;
            }
        }
    }

    __builtin_unreachable();
}

static bool table_get_record(Table const* t, size_t i, VALUE key, VALUE* value, bool* record_found)
{
    *record_found = false;

    // check if record matches the key
    if (!value_is_nil(t->items[i].key) && !value_is_tombstone(t->items[i].key) && tyc_eq(t->T, t->items[i].key, key)) {
        if (value) *value = t->items[i].value;
        *record_found = true;
        return true;
    }

    // if key is null, this ends our search
    if (value_is_nil(t->items[i].key)) {

        // key not found, let's search in the supertable first
        if (t->super && table_get(t->super, key, value)) {
            *record_found = true;
            return true;
        }

        // not found in supertable, return nil
        if (value) *value = create_value_nil();
        return true;
    }

    // if records has a key, but it's not the correct key, or it's a tombstone, we continue our search
    return false;
}

bool table_get(Table const* t, VALUE key, VALUE* value)
{
    uint32_t hash = tyc_hash(t->T, key);
    uint32_t idx = hash % t->sz;

    // loop from index to end
    bool record_found;
    for (size_t i = idx; i < t->sz; ++i) {
        bool stop = table_get_record(t, i, key, value, &record_found);
        if (stop)
            return record_found;
    }

    // loop from 0 to index
    if (idx > 0) {
        for (size_t i = 0; i < (idx-1); ++i) {
            bool stop = table_get_record(t, i, key, value, &record_found);
            if (stop)
                return record_found;
        }
    }

    __builtin_unreachable();
}

bool table_has_key(Table const* t, VALUE key)
{
    return table_get(t, key, NULL);
}

void table_del(Table* t, VALUE key)
{
    uint32_t hash = tyc_hash(t->T, key);
    uint32_t idx = hash % t->sz;

    // loop from index to end
    for (size_t i = idx; i < t->sz; ++i) {
        if (!value_is_nil(t->items[i].key) && !value_is_tombstone(t->items[i].key) && tyc_eq(t->T, t->items[i].key, key)) {
            t->items[i].key = create_value_tombstone();
            --t->in_use;
            return;
        }
    }

    // loop from 0 to index
    if (idx > 0) {
        for (size_t i = 0; i < (idx-1); ++i) {
            if (!value_is_nil(t->items[i].key) && !value_is_tombstone(t->items[i].key) && tyc_eq(t->T, t->items[i].key, key)) {
                t->items[i].key = create_value_tombstone();
                --t->in_use;
                return;
            }
        }
    }

    __builtin_unreachable();
}

bool table_next(Table const* t, VALUE key, VALUE* out_key, VALUE* out_value)
{
    // TODO - check if key was already returned on the child

    // receives a key, and looks for the next key-pair in the table
    //   it only works from start to end, it'll not circle back

    uint32_t idx;

    // received key is nil - it'll look for the first record
    if (value_is_nil(key)) {
        for (idx = 0; idx < t->sz; ++idx) {
            if (!value_is_nil(t->items[idx].key) && (!value_is_tombstone(t->items[idx].key)))
                goto found;
        }
        return false;  // not found
    }

    // received key is not nil - it'll look for the next record
    uint32_t hash = tyc_hash(t->T, key);

    for (idx = hash % t->sz + 1; idx < t->sz; ++idx)
        if (!value_is_nil(t->items[idx].key) && !value_is_tombstone(t->items[idx].key))
            goto found;

    // record was NOT found
    return false;

    // record was found, return the key/value pair
found:
    *out_key = t->items[idx].key;
    *out_value = t->items[idx].value;
    return true;
}

void table_setsuper(Table* t, Table* super)
{
    t->super = super;
}

void table_debug_internals(Table* t, TycheVM* T)
{
    for (size_t i = 0; i < t->sz; ++i) {
        printf("[%zu] -> [", i);
        tyc_debug_value(T, t->items[i].key);
        printf("]");
        if (!value_is_nil(t->items[i].key)) {
            printf(" = ");
            tyc_debug_value(T, t->items[i].value);
        }
        printf("\n");
    }
}