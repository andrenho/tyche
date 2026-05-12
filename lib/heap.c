#include "stack.c"

#include <stdlib.h>
#include <string.h>

#include "khash.h"

typedef int HEAP_KEY;

typedef enum {
    TH_STRING, TH_ARRAY, TH_TABLE,
} TYC_HEAP_TYPE;

typedef struct {
    TYC_HEAP_TYPE type;
    union {
        char* str;
        // TODO - array and table
    } value;
} HeapValue;
KHASH_MAP_INIT_INT(HEAP, HeapValue)

typedef struct {
    khash_t(HEAP) *items;
} Heap;

static void heap_init(Heap* h)
{
    h->items = kh_init(HEAP);
}

static void heap_finalize(Heap* h)
{
    for (khiter_t k = kh_begin(h->items); k != kh_end(h->items); ++k) {
        HeapValue value = kh_value(h->items, k);
        switch (value.type) {
            case TH_STRING:
                free(value.value.str);
                break;
            case TH_ARRAY:
                abort();  // not implemented yet
                break;
            case TH_TABLE:
                abort();  // not implemented yet
                break;
        }
    }
    kh_destroy(HEAP, h->items);
}

static HEAP_KEY heap_add_string(Heap* h, const char* value)
{
    int ret;
    khiter_t k;
    HEAP_KEY key;

    do {
        key = rand();
        k = kh_get(HEAP, h->items, key);
    } while (k != kh_end(h->items));

    k = kh_put(HEAP, h->items, key, &ret);
    kh_value(h->items, k) = (HeapValue) {
            .type = TH_STRING,
            .value = { .str = strdup(value) }
    };

    return key;
}

#include <stdio.h>
static TYC_RESULT heap_get_string(Heap* h, HEAP_KEY key, const char** value)
{
    khiter_t k = kh_get(HEAP, h->items, key);
    bool is_missing = (k == kh_end(h->items));
    if (is_missing)
        return T_ERR_HEAP_KEY_NOT_FOUND;
    *value = kh_value(h->items, k).value.str;
    return T_OK;
}

static size_t heap_size(Heap* h)
{
    return kh_size(h->items);
}