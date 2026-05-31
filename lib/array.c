#include "priv.h"

#include <stdlib.h>

struct Array {
    size_t n;
    size_t cap;
    VALUE* items;
};

Array* array_new(void)
{
    Array* a = xcalloc(1, sizeof(Array));
    a->n = 0;
    a->cap = 1;
    a->items = xmalloc(1 * sizeof(VALUE));
    a->items[0] = create_value_nil();
    return a;
}

void array_destroy(Array* a)
{
    free(a->items);
    free(a);
}

size_t array_len(Array const* a)
{
    return a->n;
}

VALUE array_get(Array const* a, size_t pos)
{
    if (pos >= a->n)
        return create_value_nil();
    return a->items[pos];
}

void array_set(Array* a, size_t pos, VALUE v)
{
    // extend array
    if (pos >= a->cap) {
        while (a->cap <= pos)
            a->cap *= 2;
        a->items = xrealloc(a->items, a->cap * sizeof(VALUE));
        for (size_t i = a->n; i < a->cap; ++i)
            a->items[i] = create_value_nil();
    }

    // set item
    a->items[pos] = v;
    if (pos + 1 > a->n)
        a->n = pos + 1;
}

void array_append(Array* a, VALUE v)
{
    array_set(a, a->n, v);
}
