#include "value.c"

#include <stdlib.h>

typedef struct {
    VALUE*    stack;
    size_t    stack_n;
    size_t    stack_cap;
    uint32_t* fp;
    size_t    fp_n;
    size_t    fp_cap;
} Stack;

static void stack_init(Stack* s)
{
    s->stack_n = s->fp_n = 0;
    s->stack_cap = 64;
    s->fp_cap = 8;
    s->stack = malloc(s->stack_cap * sizeof s->stack[0]);
    s->fp = malloc(s->stack_cap * sizeof s->fp[0]);

    assert(s->stack);
    assert(s->fp);
}

static void stack_destroy(Stack* s)
{
    free(s->stack);
    free(s->fp);
}

static void stack_push(Stack* s, VALUE v)
{
    if (s->stack_n == s->stack_cap) {
        s->stack_cap *= 2;
        s->stack = realloc(s->stack, s->stack_cap * sizeof s->stack[0]);
        assert(s->stack);
    }

    s->stack[s->stack_n] = v;
    ++s->stack_n;
}

static VALUE stack_pop(Stack* s)
{
}

static VALUE stack_peek(Stack* s)
{
}

static uint32_t stack_len(Stack* s)
{
}

static VALUE stack_get(Stack* s, int32_t key)
{
}

static void stack_set(Stack* s, int32_t key, VALUE v)
{
}

static void stack_push_fp(Stack* s)
{
}

static void stack_pop_fp(Stack* s)
{
}

static uint32_t stack_top_fp(Stack* s)
{
}

static uint32_t stack_fp_level(Stack* s)
{
}