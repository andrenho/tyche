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
    // TODO
}

static uint32_t find_adjusted_key(Table* t, VALUE key, bool* existing_record)
{
    uint32_t hash = tyc_hash(t->T, key);
    uint32_t idx = hash % t->sz;
    uint32_t last = (idx == 0) ? t->sz - 1 : idx - 1;

    // TODO...
}

void table_set(Table* t, VALUE key, VALUE value)
{
    if ((double) t->in_use / (double) t->sz > 0.7)
        table_rehash(t);

    uint32_t hash = tyc_hash(t->T, key);
    uint32_t idx = hash % t->sz;

    // is main slot available? use it
    if (value_is_nil(t->items[idx].kv.key)) {
        t->items[idx].kv = (TableKV) { key, value };
        ++t->in_use;

    // is the same key as the main slot?
    } else if (tyc_eq(t->T, t->items[idx].kv.key, key)) {
        t->items[idx].kv.value = value;

    // is in the extra slots?
    } else {
        bool found = false;
        for (size_t i = 0; i < t->items[idx].extra_sz; ++i) {
            if (tyc_eq(t->T, t->items[idx].extra_kv[i].key, key)) {
                t->items[idx].extra_kv[i].value = value;
                found = true;
                break;
            }
        }

        if (!found) {
            ++t->items[idx].extra_sz;
            t->items[idx].extra_kv = xrealloc(t->items[idx].extra_kv, t->items[idx].extra_sz * sizeof(TableKV));
            ++t->in_use;
        }
    }
}

TYC_RESULT table_get(Table const* t, VALUE key, VALUE* value)
{
    uint32_t hash = tyc_hash(t->T, key);
    uint32_t idx = hash % t->sz;

    if (tyc_eq(t->T, t->items[idx].kv.key, key)) {
        *value = t->items[idx].kv.value;
        return TYC_OK;
    }

    for (size_t i = 0; i < t->items[idx].extra_sz; ++i) {
        if (tyc_eq(t->T, t->items[idx].extra_kv[i].key, key)) {
            *value = t->items[idx].extra_kv[i].value;
            return TYC_OK;
        }
    }

    *value = create_value_nil();
    return TYC_OK;
}

bool table_has_key(Table const* t, VALUE key)
{
    // TODO
    return false;
}

void table_del(Table* t, VALUE key)
{
}

bool table_next(Table* t, VALUE key, VALUE* out_key, VALUE* out_value)
{
    return false;
}

void table_setsuper(Table* t, Table* super)
{
    t->super = super;

    if (super != NULL) {
        VALUE key = create_value_nil(), value;
        while (table_next(t->super, key, &key, &value)) {
            if (value_type(value) != TYC_FUNCTION)
                table_set(t, key, value);
        }
    }
}
