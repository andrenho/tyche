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

static TYC_RESULT stack_push_fp(Stack* s);

static void stack_init(Stack* s)
{
    s->stack_n = s->fp_n = 0;
    s->stack_cap = 64;
    s->fp_cap = 8;
    s->stack = malloc(s->stack_cap * sizeof s->stack[0]);
    s->fp = malloc(s->stack_cap * sizeof s->fp[0]);

    assert(s->stack);
    assert(s->fp);

    stack_push_fp(s);
}

static void stack_finalize(Stack* s)
{
    free(s->stack);
    free(s->fp);
}

static TYC_RESULT stack_push(Stack* s, VALUE v)
{
    if (s->stack_n == s->stack_cap) {
        s->stack_cap *= 2;
        s->stack = realloc(s->stack, s->stack_cap * sizeof s->stack[0]);
        assert(s->stack);
    }

    s->stack[s->stack_n] = v;
    ++s->stack_n;
    return T_OK;
}

static size_t stack_top_fp(Stack* s)
{
    return s->fp[s->fp_n - 1];
}

static TYC_RESULT stack_peek(Stack* s, VALUE* v_out)
{
    if (s->stack_n <= stack_top_fp(s))
        return T_ERR_STACK_UNDERFLOW;
    if (v_out)
        *v_out = s->stack[s->stack_n - 1];
    return T_OK;
}

static TYC_RESULT stack_pop(Stack* s, VALUE* v_out)
{
    TYC_RESULT err = stack_peek(s, v_out);
    if (err)
        return err;
    --s->stack_n;
    return T_OK;
}

static size_t stack_len(Stack* s)
{
    return s->stack_n - stack_top_fp(s);
}

static TYC_RESULT stack_at(Stack* s, int32_t key, VALUE* v)
{
    if (key >= 0) {
        if (key >= s->stack_n)
            return T_ERR_STACK_ACCESS_OUT_OF_RANGE;
        *v = s->stack[stack_top_fp(s) + key];
    } else {
        if ((int) stack_top_fp(s) + (int) s->stack_n + key < 0)
            return T_ERR_STACK_ACCESS_OUT_OF_RANGE;
        *v = s->stack[s->stack_n + key];
    }

    return T_OK;
}

static TYC_RESULT stack_set(Stack* s, int32_t key, VALUE v)
{
    abort(); // TODO
}

static TYC_RESULT stack_push_fp(Stack* s)
{
    if (s->fp_n == s->fp_cap) {
        s->fp_cap *= 2;
        s->fp = realloc(s->fp, s->fp_cap * sizeof s->fp[0]);
        assert(s->fp);
    }

    s->fp[s->fp_n] = (uint32_t) s->stack_n;
    ++s->fp_n;
    return T_OK;
}

static TYC_RESULT stack_pop_fp(Stack* s)
{
    if (s->fp_n == 1)
        return T_ERR_STACK_FP_UNDERFLOW;
    s->stack_n = stack_top_fp(s);
    --s->fp_n;
    return T_OK;
}

static size_t stack_fp_level(Stack* s)
{
    return s->fp_n;
}