#include "stack.c"

#include <stdlib.h>

#include "khash.h"

typedef struct {
} HeapValue;
KHASH_MAP_INIT_INT(heap, HeapValue)

typedef struct {
    khash_t(heap) *items;
} Heap;

static void heap_init(Heap* h)
{
    h->items = kh_init(heap);
}

static void heap_finalize(Heap* h)
{
    kh_destroy(heap, h->items);
}
