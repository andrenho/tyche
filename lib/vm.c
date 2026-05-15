#include "priv.h"

#include <stdlib.h>

struct TycheVM {
    Stack* stack;
    Heap*  heap;
    Code*  code;
};

TycheVM* tyc_new(void)
{
    TycheVM* t = xcalloc(1, sizeof(TycheVM));
    t->stack = stack_new();
    t->heap = heap_new();
    t->code = code_new();
    return t;
}

void tyc_destroy(TycheVM* t)
{
    code_destroy(t->code);
    heap_destroy(t->heap);
    stack_destroy(t->stack);
    free(t);
}

TYC_RESULT tyc_load_bytecode(TycheVM* T, uint8_t const* bytecode, size_t bytecode_sz)
{
    return code_load_bytecode(T->code, bytecode, bytecode_sz);
}

TYC_RESULT tyc_call(TycheVM* T, uint16_t n_pars)
{
    abort();  // TODO
}

size_t tyc_stack_size(TycheVM* T)
{
    return stack_len(T->stack);
}

void tyc_pushinteger(TycheVM* T, int32_t value)
{
    stack_push(T->stack, create_value_integer(value));
}

TYC_RESULT tyc_type(TycheVM* T, int idx, TYC_TYPE* type)
{
    VALUE v;
    TYC_RESULT r = stack_at(T->stack, idx, &v);
    if (r == T_OK)
        *type = v.type;
    return r;
}

TYC_RESULT tyc_tointeger(TycheVM* T, int idx, int32_t* value)
{
    VALUE v;
    TYC_RESULT r = stack_at(T->stack, idx, &v);
    if (r == T_OK) {
        if (v.type != TT_INTEGER)
            return T_ERR_TYPE_UNEXPECTED;
        *value = value_integer(v);
    }
    return r;
}

TYC_RESULT tyc_expr(TycheVM* T, TYC_EXPR expr)
{
    // TODO
    VALUE v1, v2;
    stack_pop(T->stack, &v2);
    stack_pop(T->stack, &v1);
    stack_push(T->stack, create_value_integer(value_integer(v1) + value_integer(v2)));
    return T_OK;
}