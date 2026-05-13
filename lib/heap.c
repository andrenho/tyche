#include "priv.h"

#include <stdlib.h>
#include <string.h>

#include "khash.h"

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
KHASH_MAP_INIT_INT64(HEAP, HeapValue)
KHASH_MAP_INIT_INT64(MARK, bool)
#pragma GCC diagnostic pop

struct Heap {
    khash_t(HEAP) *items;
};

Heap* heap_new(void)
{
    Heap* h = xcalloc(1, sizeof(Heap));
    h->items = kh_init(HEAP);
    return h;
}

static void heap_free_item(HeapValue value)
{
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
    free(h);
}

HEAP_KEY heap_add_string(Heap* h, const char* value)
{
    int ret;
    khiter_t k;
    HEAP_KEY key;

    do {
        key = (HEAP_KEY) rand();
        k = kh_get(HEAP, h->items, key);
    } while (k != kh_end(h->items));

    k = kh_put(HEAP, h->items, key, &ret);
    kh_value(h->items, k) = (HeapValue) {
            .type = TH_STRING,
            .value = { .str = strdup(value) }
    };

    return key;
}

TYC_RESULT heap_get_string(Heap* h, HEAP_KEY key, const char** value)
{
    khiter_t k = kh_get(HEAP, h->items, key);
    bool is_missing = (k == kh_end(h->items));
    if (is_missing)
        return T_ERR_HEAP_KEY_NOT_FOUND;
    *value = kh_value(h->items, k).value.str;
    return T_OK;
}

size_t heap_size(Heap* h)
{
    return kh_size(h->items);
}

//
// GC
//

void heap_gc(Heap* h, VALUE const* roots, size_t n_roots)
{
    //
    // mark
    //

    khash_t(MARK) *marked = kh_init(MARK);

    for (size_t i = 0; i < n_roots; ++i) {
        if (value_type(roots[i]) == TT_STRING) {
            int ret;
            uint32_t key = value_idx(roots[i]);
            khiter_t k = kh_put(MARK, marked, key, &ret);
            if (ret < 0)
                out_of_memory();
            kh_value(marked, k) = true;
        }
    }

    //
    // sweep
    //

    for (khiter_t k = kh_begin(h->items); k != kh_end(h->items); ++k) {
        if (kh_exist(h->items, k)) {
            HEAP_KEY key = (HEAP_KEY) kh_key(h->items, k);
            if (kh_get(MARK, marked, key) == kh_end(marked)) {
                khiter_t kk = kh_get(HEAP, h->items, key);
                heap_free_item(kh_value(h->items, kk));
                kh_del(HEAP, h->items, kk);
            }
        }
    }

    kh_destroy(MARK, marked);
}