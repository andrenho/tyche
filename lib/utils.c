#include "priv.h"

#include <stdlib.h>
#include <stdio.h>

__attribute__((noreturn)) void out_of_memory(void)
{
    fprintf(stderr, "out of memory\n");
    abort();
}

void* xmalloc(size_t n)
{
    void* p = malloc(n);
    if (!p) out_of_memory();
    return p;
}

void* xcalloc(size_t n, size_t size)
{
    void* p = calloc(n, size);
    if (!p) out_of_memory();
    return p;
}

void* xrealloc(void* p, size_t n)
{
    void* q = realloc(p, n);
    if (!q) out_of_memory();
    return q;
}
