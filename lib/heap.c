#include "priv.h"

#include <stdlib.h>
#include <string.h>

#include "khash.h"

#define TRY(x) if ((r = (x)) != T_OK) { return r; }

typedef enum {
    TH_STRING, TH_ARRAY, TH_TABLE,
} TYC_HEAP_TYPE;

typedef struct {
    HEAP_KEY key;
    bool     constant;
} StringValue;

typedef struct {
    TYC_HEAP_TYPE type;
    union {
        char*  string;
        Array* array;
        Table* table;
    } value;
} HeapValue;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
KHASH_MAP_INIT_INT64(HEAP, HeapValue)
KHASH_MAP_INIT_STR(STR, StringValue)
KHASH_MAP_INIT_INT64(MARK, bool)
#pragma GCC diagnostic pop

struct Heap {
    khash_t(HEAP) *items;
    khash_t(STR)  *strings;
};

Heap* heap_new(void)
{
    Heap* h = xcalloc(1, sizeof(Heap));
    h->items = kh_init(HEAP);
    h->strings = kh_init(STR);
    return h;
}

static void heap_free_item(HeapValue value)
{
    switch (value.type) {
        case TH_STRING:
            free(value.value.string);
            break;
        case TH_ARRAY:
            array_destroy(value.value.array);
            break;
        case TH_TABLE:
            table_destroy(value.value.table);
            break;
        default:
            __builtin_unreachable();
    }
}

void heap_destroy(Heap* h)
{
    for (khiter_t k = kh_begin(h->items); k != kh_end(h->items); ++k) {
        if (kh_exist(h->items, k)) {
            HeapValue value = kh_value(h->items, k);
            heap_free_item(value);
        }
    }
    kh_destroy(HEAP, h->items);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    for (khiter_t k = kh_begin(h->strings); k != kh_end(h->strings); ++k)
        if (kh_exist(h->strings, k))
            free((char *) kh_key(h->strings, k));
#pragma GCC diagnostic pop
    kh_destroy(STR, h->strings);

    free(h);
}

static HeapValue* heap_add_item(Heap* h, HEAP_KEY* key)
{
    int ret;
    khiter_t k;

    do {
        *key = (HEAP_KEY) rand();
        k = kh_get(HEAP, h->items, *key);
    } while (k != kh_end(h->items));

    k = kh_put(HEAP, h->items, *key, &ret);
    if (ret < 0)
        out_of_memory();

    return &kh_value(h->items, k);
}

static TYC_RESULT heap_get_item(Heap const* h, HEAP_KEY key, HeapValue** out_value)
{
    khiter_t k = kh_get(HEAP, h->items, key);
    bool is_missing = (k == kh_end(h->items));
    if (is_missing)
        return T_ERR_HEAP_KEY_NOT_FOUND;
    *out_value = &kh_value(h->items, k);
    return T_OK;
}

HEAP_KEY heap_add_string(Heap* h, const char* value, bool constant)
{
    // look in string list first
    khiter_t k = kh_get(STR, h->strings, value);
    if (k != kh_end(h->strings)) {  // found
        if (constant)
            kh_value(h->strings, k).constant = true;
        return kh_value(h->strings, k).key;
    }

    // not found in the string list, create it
    HEAP_KEY key;
    *heap_add_item(h, &key) = (HeapValue) {
        .type = TH_STRING,
        .value = { .string = strdup(value) }
    };

    int ret;
    k = kh_put(STR, h->strings, strdup(value), &ret);
    if (ret < 0)
        out_of_memory();
    kh_value(h->strings, k) = (StringValue) { .key = key, .constant = constant };

    return key;
}

TYC_RESULT heap_get_string(Heap const* h, HEAP_KEY key, const char** value)
{
    TYC_RESULT r;
    HeapValue* hv;

    TRY(heap_get_item(h, key, &hv))
    if (hv->type != TH_STRING)
        abort();
    *value = hv->value.string;

    return T_OK;
}

HEAP_KEY heap_add_array(Heap* h)
{
    HEAP_KEY key;
    *heap_add_item(h, &key) = (HeapValue) {
        .type = TH_ARRAY,
        .value = { .array = array_new() },
    };
    return key;
}

TYC_RESULT heap_get_array(Heap const* h, HEAP_KEY key, Array** array)
{
    TYC_RESULT r;
    HeapValue* hv;

    TRY(heap_get_item(h, key, &hv))
    if (hv->type != TH_ARRAY)
        abort();
    *array = hv->value.array;

    return T_OK;
}

HEAP_KEY heap_add_table(Heap* h)
{
    HEAP_KEY key;
    *heap_add_item(h, &key) = (HeapValue) {
        .type = TH_TABLE,
        .value = { .table = table_new(h) },
    };
    return key;
}

TYC_RESULT heap_get_table(Heap const* h, HEAP_KEY key, Table** table)
{
    TYC_RESULT r;
    HeapValue* hv;

    TRY(heap_get_item(h, key, &hv))
    if (hv->type != TH_TABLE)
        abort();
    *table = hv->value.table;

    return T_OK;
}

size_t heap_size(Heap const* h)
{
    return kh_size(h->items);
}

//
// GC
//

struct TableIter {
    Heap* h;
    khash_t(MARK)* marked;
};

static void mark(Heap* h, VALUE a, khash_t(MARK)* marked);

static void mark_table_item(VALUE key, VALUE value, void* data)
{
    struct TableIter* it = (struct TableIter *) data;
    mark(it->h, key, it->marked);
    mark(it->h, value, it->marked);
}

static void mark(Heap* h, VALUE a, khash_t(MARK)* marked)
{
    if (type_is_collectable(value_type(a))) {
        int ret;
        uint32_t key = value_heap_key(a);
        khiter_t k = kh_put(MARK, marked, key, &ret);
        if (ret < 0)
            out_of_memory();
        kh_value(marked, k) = true;
    }

    if (value_type(a) == TT_ARRAY) {
        Array* array;
        if (heap_get_array(h, value_heap_key(a), &array) != T_OK)
            abort();
        size_t len = array_len(array);
        for (size_t i = 0; i < len; ++i) {
            VALUE item = array_get(array, i);
            mark(h, item, marked);
        }
    } else if (value_type(a) == TT_TABLE) {
        Table* table;
        if (heap_get_table(h, value_heap_key(a), &table) != T_OK)
            abort();
        table_foreach(table, mark_table_item, &(struct TableIter) { h, marked });
    }
}

void heap_gc(Heap* h, VALUE const* roots, size_t n_roots)
{
    //
    // mark
    //

    khash_t(MARK) *marked = kh_init(MARK);

    for (size_t i = 0; i < n_roots; ++i)
        mark(h, roots[i], marked);

    //
    // sweep
    //

    for (khiter_t k = kh_begin(h->items); k != kh_end(h->items); ++k) {
        if (kh_exist(h->items, k)) {
            HEAP_KEY key = (HEAP_KEY) kh_key(h->items, k);
            if (kh_get(MARK, marked, key) == kh_end(marked)) {

                // delete from strings
                if (kh_value(h->items, k).type == TH_STRING) {
                    khiter_t kk = kh_get(STR, h->strings, kh_value(h->items, k).value.string);
                    if (kk != kh_end(h->strings)) {
                        if (kh_value(h->strings, kk).constant)  // never delete constant
                            continue;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
                        free((char *) kh_key(h->strings, kk));
                        kh_del(STR, h->strings, kk);
#pragma GCC diagnostic pop
                    }
                }

                // delete from list
                khiter_t kk = kh_get(HEAP, h->items, key);
                heap_free_item(kh_value(h->items, kk));
                kh_del(HEAP, h->items, kk);
            }
        }

        // TODO - delete from string list
    }

    kh_destroy(MARK, marked);
}