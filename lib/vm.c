#include "priv.h"

#include <stdlib.h>

typedef struct Location {
    uint32_t function_id;
    uint32_t pc;
} Location;

typedef struct LocationStack {
    Location* locations;
    size_t    sz;
    size_t    cap;
} LocationStack;

struct TycheVM {
    Stack*        stack;
    Heap*         heap;
    Code*         code;
    LocationStack location_stack;
};

//
// CREATE/DESTROY VM
//

TycheVM* tyc_new(void)
{
    TycheVM* t = xcalloc(1, sizeof(TycheVM));
    t->stack = stack_new();
    t->heap = heap_new();
    t->code = code_new();
    t->location_stack = (LocationStack) {
        .locations = xmalloc(4 * sizeof(Location)),
        .cap = 4,
        .sz = 0,
    };
    return t;
}

void tyc_destroy(TycheVM* t)
{
    free(t->location_stack.locations);
    code_destroy(t->code);
    heap_destroy(t->heap);
    stack_destroy(t->stack);
    free(t);
}

//
// LOCATION STACK
//

static void push_location(TycheVM* T, uint32_t function_id, uint32_t pc)
{
    if (T->location_stack.sz == T->location_stack.cap) {
        T->location_stack.cap *= 2;
        T->location_stack.locations = xrealloc(T->location_stack.locations, T->location_stack.cap * sizeof(Location));
    }

    T->location_stack.locations[T->location_stack.sz] = (Location) {
        .function_id = function_id,
        .pc = pc,
    };
    ++T->location_stack.sz;
}

//
// CODE LOADING AND EXECUTION
//

TYC_RESULT tyc_load_bytecode(TycheVM* T, uint8_t const* bytecode, size_t bytecode_sz)
{
    TYC_RESULT r;
    if ((r = code_load_bytecode(T->code, bytecode, bytecode_sz)) != T_OK)
        return r;
    if ((r = stack_push(T->stack, create_value_idx(TT_FUNCTION, 0 /* main */))) != T_OK)
        return r;
    return T_OK;
}

static TYC_RESULT enter_function(TycheVM* T, uint16_t n_pars)
{
    TYC_RESULT r;

    // get parameters
    VALUE params[n_pars + 1];
    for (uint16_t i = 0; i < n_pars; ++i)
        if ((r = stack_pop(T->stack, &params[i])) != T_OK)
            return r;

    // get function
    VALUE function;
    if ((r = stack_pop(T->stack, &function)) != T_OK)
        return r;
    if (value_type(function) != TT_FUNCTION)
        return T_ERR_TYPE_UNEXPECTED;

    // enter function
    push_location(T, value_idx(function), 0);
    stack_push_fp(T->stack);

    // pass parameters
    for (int i = n_pars-1; i >= 0; --i)
        if ((r = stack_push(T->stack, params[i])) != T_OK)
            return r;

    return T_OK;
}

static TYC_RESULT run_until_return(TycheVM* T)
{
    TYC_RESULT r;

    size_t level = stack_fp_level(T->stack);
    while (stack_fp_level(T->stack) >= level)
        if ((r = step(T)) != T_OK)
            return r;

    return T_OK;
}

TYC_RESULT tyc_call(TycheVM* T, uint16_t n_pars)
{
    TYC_RESULT r;
    if ((r = enter_function(T, n_pars)) != T_OK)
        return r;
    if ((r = run_until_return(T)) != T_OK)
        return r;
    return T_OK;
}

//
// STACK MANIPULATION AND QUERY
//

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