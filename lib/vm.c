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

static TYC_RESULT step(TycheVM* T);

#define TRY(x) if ((r = (x)) != T_OK) { return r; }

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

    expr_init();

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

static Location* location_top(TycheVM* T)
{
    if (T->location_stack.sz == 0)
        abort();

    return &T->location_stack.locations[T->location_stack.sz - 1];
}

static void location_pop(TycheVM* T)
{
    if (T->location_stack.sz == 0)
        abort();

    --T->location_stack.sz;
}

//
// CODE LOADING AND EXECUTION
//

TYC_RESULT tyc_load_bytecode(TycheVM* T, uint8_t const* bytecode, size_t bytecode_sz)
{
    TYC_RESULT r;
    TRY(code_load_bytecode(T->code, bytecode, bytecode_sz))
    TRY(stack_push(T->stack, create_value_idx(TT_FUNCTION, 0 /* main */)))
    return T_OK;
}

static TYC_RESULT enter_function(TycheVM* T, uint16_t n_pars)
{
    TYC_RESULT r;

    // get parameters
    VALUE params[n_pars + 1];
    for (uint16_t i = 0; i < n_pars; ++i)
        TRY(stack_pop(T->stack, &params[i]))

    // get function
    VALUE function;
    TRY(stack_pop(T->stack, &function))
    if (value_type(function) != TT_FUNCTION)
        return T_ERR_TYPE_UNEXPECTED;

    // enter function
    push_location(T, value_idx(function), 0);
    stack_push_fp(T->stack);

    // pass parameters
    for (int i = n_pars-1; i >= 0; --i)
        TRY(stack_push(T->stack, params[i]))

    return T_OK;
}

static TYC_RESULT run_until_return(TycheVM* T)
{
    TYC_RESULT r;

    size_t level = stack_fp_level(T->stack);
    while (stack_fp_level(T->stack) >= level)
        TRY(step(T))

    return T_OK;
}

TYC_RESULT tyc_call(TycheVM* T, uint16_t n_pars)
{
    TYC_RESULT r;
    TRY(enter_function(T, n_pars))
    TRY(run_until_return(T))
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

TYC_RESULT tyc_expr(TycheVM* T, TYC_EXPR op)
{
    TYC_RESULT r;
    VALUE v1, v2, result;

    stack_pop(T->stack, &v2);
    stack_pop(T->stack, &v1);
    TRY(binary_expr(op, v1, v2, &result))
    stack_push(T->stack, result);

    return T_OK;
}

//
// STEP
//

static TYC_RESULT step(TycheVM* T)
{
    VALUE a;
    TYC_RESULT r;

    Location* loc = location_top(T);
    Instruction inst = code_next_instruction(T->code, loc->function_id, loc->pc);

    // TODO - debug instruction

    switch (inst.operator) {

        //
        // stack manipulation
        //

        case TO_PUSHI:
            tyc_pushinteger(T, inst.operand);
            break;

        //
        // expressions
        //

        case TO_SUM:  TRY(tyc_expr(T, TX_SUM));  break;
        case TO_SUB:  TRY(tyc_expr(T, TX_SUB));  break;
        case TO_MUL:  TRY(tyc_expr(T, TX_MUL));  break;
        case TO_IDIV: TRY(tyc_expr(T, TX_IDIV)); break;
        case TO_EQ:   TRY(tyc_expr(T, TX_EQ));   break;
        case TO_NEQ:  TRY(tyc_expr(T, TX_NEQ));  break;
        case TO_LT:   TRY(tyc_expr(T, TX_LT));   break;
        case TO_LTE:  TRY(tyc_expr(T, TX_LTE));  break;
        case TO_GT:   TRY(tyc_expr(T, TX_GT));   break;
        case TO_GTE:  TRY(tyc_expr(T, TX_GTE));  break;
        case TO_AND:  TRY(tyc_expr(T, TX_AND));  break;
        case TO_OR:   TRY(tyc_expr(T, TX_OR));   break;
        case TO_XOR:  TRY(tyc_expr(T, TX_XOR));  break;
        case TO_POW:  TRY(tyc_expr(T, TX_POW));  break;
        case TO_SHL:  TRY(tyc_expr(T, TX_SHL));  break;
        case TO_SHR:  TRY(tyc_expr(T, TX_SHR));  break;
        case TO_MOD:  TRY(tyc_expr(T, TX_MOD));  break;

        //
        // function calls
        //

        case TO_RET:
            TRY(stack_pop(T->stack, &a))
            TRY(stack_pop_fp(T->stack))
            TRY(stack_push(T->stack, a))
            location_pop(T);
            // TODO - print stack
            return T_OK;

        default:
            return T_ERR_INVALID_OPCODE;
    }

    // TODO - print stack
    loc->pc += inst.sz;

    return T_OK;
}
