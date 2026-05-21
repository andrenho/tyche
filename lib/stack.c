#include "priv.h"

#include <assert.h>
#include <stdlib.h>

struct Stack {
    VALUE*    stack;
    size_t    stack_n;
    size_t    stack_cap;
    uint32_t* fp;
    size_t    fp_n;
    size_t    fp_cap;
};

Stack* stack_new(void)
{
    Stack* s = xcalloc(1, sizeof(Stack));

    s->stack_n = 0;
    s->fp_n = 0;
    s->stack_cap = 64;
    s->fp_cap = 8;
    s->stack = xmalloc(s->stack_cap * sizeof s->stack[0]);
    s->fp = xmalloc(s->stack_cap * sizeof s->fp[0]);

    assert(s->stack);
    assert(s->fp);

    stack_push_fp(s);

    return s;
}

void stack_destroy(Stack* s)
{
    free(s->stack);
    free(s->fp);
    free(s);
}

TYC_RESULT stack_push(Stack* s, VALUE v)
{
    if (s->stack_n == s->stack_cap) {
        s->stack_cap *= 2;
        s->stack = xrealloc(s->stack, s->stack_cap * sizeof s->stack[0]);
        assert(s->stack);
    }

    s->stack[s->stack_n] = v;
    ++s->stack_n;
    return T_OK;
}

size_t stack_top_fp(Stack const* s)
{
    return s->fp[s->fp_n - 1];
}

TYC_RESULT stack_peek(Stack const* s, VALUE* v_out)
{
    if (s->stack_n <= stack_top_fp(s))
        return T_ERR_STACK_UNDERFLOW;
    if (v_out)
        *v_out = s->stack[s->stack_n - 1];
    return T_OK;
}

TYC_RESULT stack_pop(Stack* s, VALUE* v_out)
{
    TYC_RESULT err = stack_peek(s, v_out);
    if (err)
        return err;
    --s->stack_n;
    return T_OK;
}

size_t stack_size(Stack const* s)
{
    return s->stack_n - stack_top_fp(s);
}

TYC_RESULT stack_at(Stack const* s, int32_t key, VALUE* v)
{
    if (key >= 0) {
        if ((int) stack_top_fp(s) + key >= (int) s->stack_n)
            return T_ERR_STACK_ACCESS_OUT_OF_RANGE;
        *v = s->stack[(int) stack_top_fp(s) + key];
    } else {
        if ((int) s->stack_n + key < (int) stack_top_fp(s))
            return T_ERR_STACK_ACCESS_OUT_OF_RANGE;
        *v = s->stack[(int) s->stack_n + key];
    }

    return T_OK;
}

TYC_RESULT stack_set(Stack* s, int32_t key, VALUE v)
{
    if (key >= 0) {
        if ((int) stack_top_fp(s) + key >= (int) s->stack_n)
            return T_ERR_STACK_ACCESS_OUT_OF_RANGE;
        s->stack[(int) stack_top_fp(s) + key] = v;
    } else {
        if ((int) s->stack_n + key < (int) stack_top_fp(s))
            return T_ERR_STACK_ACCESS_OUT_OF_RANGE;
        s->stack[(int) s->stack_n + key] = v;
    }

    return T_OK;
}

TYC_RESULT stack_push_fp(Stack* s)
{
    if (s->fp_n == s->fp_cap) {
        s->fp_cap *= 2;
        s->fp = xrealloc(s->fp, s->fp_cap * sizeof s->fp[0]);
        assert(s->fp);
    }

    s->fp[s->fp_n] = (uint32_t) s->stack_n;
    ++s->fp_n;
    return T_OK;
}

TYC_RESULT stack_pop_fp(Stack* s)
{
    if (s->fp_n == 1)
        return T_ERR_STACK_FP_UNDERFLOW;
    s->stack_n = stack_top_fp(s);
    --s->fp_n;
    return T_OK;
}

size_t stack_fp_level(Stack const* s)
{
    return s->fp_n;
}

size_t stack_collectable_array(Stack const* s, VALUE** values)
{
    size_t j = 0;
    *values = xmalloc(stack_size(s) * sizeof(VALUE));

    for (size_t i = 0; i < s->stack_n; ++i)
        if (type_is_collectable(s->stack[i].type))
            (*values)[j++] = s->stack[i];
    return j;
}
