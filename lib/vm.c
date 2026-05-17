#include "priv.h"

#include <stdlib.h>
#include <stdio.h>

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
    bool          debug;
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
    t->debug = false;

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
// DEBUGGING
//

void tyc_debug_to_console(TycheVM* T, bool activate)
{
    T->debug = activate;
}

#ifdef DEBUG_ASSEMBLY

static void debug_instruction(TycheVM* T, Location* loc, Instruction inst)
{
    if (!T->debug)
        return;

    char buf[50];
    code_parse_instruction(inst, buf, sizeof(buf));
    printf(": %02d-%04d   %s    ", loc->function_id, loc->pc, buf);
}

static void debug_value(TycheVM* T, VALUE a)
{
    switch (value_type(a)) {
        case TT_NIL:
            printf("[nil]");
            break;
        case TT_INTEGER:
            printf("[%d]", value_integer(a));
            break;
        case TT_REAL:
            printf("[%f]", (double) value_real(a));
            break;
        case TT_STRING: {
            const char* str;
            if (heap_get_string(T->heap, value_idx(a), &str) == T_OK)
                printf("[\"%s\"]", str);
            else
                printf("[\"(not found)\"]");
            break;
        }
        case TT_STRING_CONST: {
            if (code_const_type(T->code, value_idx(a)) != TC_STRING)
                printf("[\"(const not a string)\"]");
            else
                printf("[\"%s\"]", code_const_string(T->code, value_idx(a)));
            break;
        }
        case TT_ARRAY:
            printf("[(not implemented)]\n");
            abort();
        case TT_TABLE:
            printf("[(not implemented )]\n");
            abort();
        case TT_FUNCTION:
            printf("[func %d]", value_idx(a));
            break;
        case TT_NATIVE_PTR:
            printf("[ptr %p]", (void *) (intptr_t) value_idx(a));
            break;
        case TT_COUNT__:
            __builtin_unreachable();
    }
}

static void debug_stack(TycheVM* T)
{
    if (!T->debug)
        return;
    if (stack_size(T->stack) == 0) {
        printf("|empty|\n");
        return;
    }
    for (size_t i = 0; i < stack_size(T->stack); ++i) {
        VALUE a;
        stack_at(T->stack, (int32_t) i, &a);
        debug_value(T, a);
        printf(" ");
    }
    printf("\n");
}

void tyc_assembly_decompile(TycheVM* T)
{
    code_decompile(T->code);
}

#endif

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
    VALUE* params = xcalloc(n_pars + 1, sizeof(VALUE));
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

    free(params);
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
    return stack_size(T->stack);
}

void tyc_pushnil(TycheVM* T)
{
    stack_push(T->stack, create_value_nil());
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

#ifdef DEBUG_ASSEMBLY
    debug_instruction(T, loc, inst);
#endif

    switch (inst.operator) {

        //
        // stack manipulation
        //

        case TO_PUSHN:
            tyc_pushnil(T);
            break;

        case TO_PUSHI:
            tyc_pushinteger(T, inst.operand);
            break;

        case TO_PUSHF:
            if (inst.operand < 0 || inst.operand > (int) code_n_functions(T->code))
                return T_ERR_VALUE_OUT_OF_RANGE;
            TRY(stack_push(T->stack, create_value_idx(TT_FUNCTION, (uint32_t) inst.operand)))
            break;

        case TO_POP:
            TRY(stack_pop(T->stack, NULL))
            break;

        //
        // local variables
        //

        case TO_PUSHV:
            if (inst.operand <= 0)
                return T_ERR_VALUE_OUT_OF_RANGE;
            for (int i = 0; i < inst.operand; ++i)
                tyc_pushnil(T);
            break;

        case TO_SET:
            if (inst.operand < 0)
                return T_ERR_VALUE_OUT_OF_RANGE;
            TRY(stack_pop(T->stack, &a))
            TRY(stack_set(T->stack, inst.operand, a))
            break;

        case TO_DUPV:
            if (inst.operand < 0)
                return T_ERR_VALUE_OUT_OF_RANGE;
            TRY(stack_at(T->stack, inst.operand, &a))
            stack_push(T->stack, a);
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

        case TO_CALL:
            if (inst.operand < 0)
                return T_ERR_VALUE_OUT_OF_RANGE;
            enter_function(T, (uint16_t) inst.operand);
            break;

        case TO_RET:
            TRY(stack_pop(T->stack, &a))
            TRY(stack_pop_fp(T->stack))
            TRY(stack_push(T->stack, a))
            location_pop(T);
            goto dont_update_pc;

        //
        // jumps/branching
        //

        case TO_JMP:
            if (inst.operand < 0)
                return T_ERR_VALUE_OUT_OF_RANGE;  // TODO - also check function size
            loc->pc = (uint32_t) inst.operand;
            goto dont_update_pc;

        case TO_BZ:
            if (inst.operand < 0)
                return T_ERR_VALUE_OUT_OF_RANGE;  // TODO - also check function size
            TRY(stack_pop(T->stack, &a))
            if (value_is_zero(a)) {
                loc->pc = (uint32_t) inst.operand;
                goto dont_update_pc;
            }
            break;

        case TO_BNZ:
            if (inst.operand < 0)
                return T_ERR_VALUE_OUT_OF_RANGE;  // TODO - also check function size
            TRY(stack_pop(T->stack, &a))
            if (!value_is_zero(a)) {
                loc->pc = (uint32_t) inst.operand;
                goto dont_update_pc;
            }
            break;

        default:
            return T_ERR_INVALID_OPCODE;
    }

    // TODO - print stack
    loc->pc += inst.sz;

dont_update_pc:
#ifdef DEBUG_ASSEMBLY
    debug_stack(T);
#endif
    return T_OK;
}
