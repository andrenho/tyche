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
    VALUE         global_table;
    bool          debug;
};

static TYC_RESULT step(TycheVM* T);

//
// ERROR MANAGEMENT
//

__thread char last_err_msg[256] = {0};

bool abort_on_errors = true;  // only in debug mode

const char* tyc_last_error(void)
{
    return last_err_msg;
}

//
// CREATE/DESTROY VM
//

TycheVM* tyc_new(void)
{
    TycheVM* T = xcalloc(1, sizeof(TycheVM));
    T->stack = stack_new();
    T->heap = heap_new();
    T->code = code_new();
    T->location_stack = (LocationStack) {
        .locations = xmalloc(4 * sizeof(Location)),
        .cap = 4,
        .sz = 0,
    };
    T->global_table = create_value_heap_key(TT_TABLE, heap_add_table(T->heap));
    T->debug = false;

    expr_init();

    return T;
}

void tyc_destroy(TycheVM* T)
{
    heap_gc(T->heap, NULL, 0);

    free(T->location_stack.locations);
    code_destroy(T->code);
    heap_destroy(T->heap);
    stack_destroy(T->stack);
    free(T);
}

//
// PRIVATE METHODS
//

Stack* tyc_stack(TycheVM* T) { return T->stack; }
Heap*  tyc_heap(TycheVM* T) { return T->heap; }
Code*  tyc_code(TycheVM* T) { return T->code; }

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
            printf("<nil>");
            break;
        case TT_BOOLEAN:
            if (value_boolean(a)) printf("<true>"); else printf("<false>");
            break;
        case TT_INTEGER:
            printf("<%d>", value_integer(a));
            break;
        case TT_REAL:
            printf("<%f>", (double) value_real(a));
            break;
        case TT_STRING: {
            const char* str;
            if (heap_get_string(T->heap, value_heap_key(a), &str) == T_OK)
                printf("<\"%s\">", str);
            else
                printf("<\"(not found)\">");
            break;
        }
        case TT_ARRAY: {
            Array* array;
            if (heap_get_array(T->heap, value_heap_key(a), &array) == T_OK) {
                printf("[");
                for (size_t i = 0; i < array_len(array); ++i) {
                    debug_value(T, array_get(array, i));
                    if (i < array_len(array) - 1)
                        printf(", ");
                }
                printf("]");
            } else {
                printf("[(not found)]\n");
            }
            break;
        }
        case TT_TABLE: {
            Table* table;
            if (heap_get_table(T->heap, value_heap_key(a), &table) == T_OK) {
                printf("{");
                VALUE key = create_value_nil();
                VALUE value;
                while (table_next(table, key, &key, &value)) {
                    debug_value(T, key);
                    printf(":");
                    debug_value(T, value);
                    printf(", ");
                }
                printf("}");
            } else {
                printf("{(not implemented )}\n");
            }
            break;
        }
        case TT_FUNCTION:
            printf("<func %d>", value_function_idx(a));
            break;
        case TT_NATIVE_PTR:
            printf("<ptr %p>", (void *) (intptr_t) value_function_idx(a));
            break;
        case TT_COUNT__:
        default:
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

void tyc_print_bytecode(TycheVM* T)
{
    code_debug_bytecode(T->code);
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
    TRY(stack_push(T->stack, create_value_function_idx(0 /* main */)))
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
        ERROR("Expected function")

    // enter function
    push_location(T, value_function_idx(function), 0);
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

TYC_RESULT tyc_pushnil(TycheVM* T)
{
    return stack_push(T->stack, create_value_nil());
}

TYC_RESULT tyc_pushboolean(TycheVM* T, bool value)
{
    return stack_push(T->stack, create_value_bool(value));
}

TYC_RESULT tyc_pushinteger(TycheVM* T, int32_t value)
{
    return stack_push(T->stack, create_value_integer(value));
}

TYC_RESULT tyc_pushstring(TycheVM* T, const char* value)
{
    return stack_push(T->stack, create_value_heap_key(TT_STRING, heap_add_string(T->heap, value, false)));
}

TYC_RESULT tyc_newarray(TycheVM* T)
{
    return stack_push(T->stack, create_value_heap_key(TT_ARRAY, heap_add_array(T->heap)));
}

TYC_RESULT tyc_newtable(TycheVM* T)
{
    return stack_push(T->stack, create_value_heap_key(TT_TABLE, heap_add_table(T->heap)));
}

TYC_RESULT tyc_type(TycheVM* T, int idx, TYC_TYPE* type)
{
    VALUE v;
    TYC_RESULT r = stack_at(T->stack, idx, &v);
    if (r == T_OK)
        *type = value_type(v);
    return r;
}

TYC_RESULT tyc_toboolean(TycheVM* T, int idx, bool* value)
{
    VALUE v;
    TYC_RESULT r;
    TRY(stack_at(T->stack, idx, &v))
    if (value_type(v) != TT_BOOLEAN)
    ERROR("Expected boolean")
    *value = value_boolean(v);
    return T_OK;
}

TYC_RESULT tyc_tointeger(TycheVM* T, int idx, int32_t* value)
{
    VALUE v;
    TYC_RESULT r;
    TRY(stack_at(T->stack, idx, &v))
    if (value_type(v) != TT_INTEGER)
        ERROR("Expected integer")
    *value = value_integer(v);
    return T_OK;
}

TYC_RESULT tyc_toreal(TycheVM* T, int idx, T_REAL* value)
{
    VALUE v;
    TYC_RESULT r;
    TRY(stack_at(T->stack, idx, &v))
    if (value_type(v) != TT_REAL)
    ERROR("Expected real")
    *value = value_real(v);
    return T_OK;
}

TYC_RESULT tyc_tostring(TycheVM* T, int idx, const char** str)
{
    VALUE v;
    TYC_RESULT r;
    TRY(stack_at(T->stack, idx, &v))
    if (value_type(v) == TT_STRING)
        return heap_get_string(T->heap, value_heap_key(v), str);
    else
        ERROR("Expected string")
    return T_OK;
}

TYC_RESULT tyc_expr(TycheVM* T, TYC_EXPR op)
{
    TYC_RESULT r;
    VALUE v1, v2, result;

    if (expr_is_binary(op))
        stack_pop(T->stack, &v2);
    else
        v2 = create_value_nil();
    stack_pop(T->stack, &v1);
    TRY(binary_expr(T, op, v1, v2, &result))
    stack_push(T->stack, result);

    return T_OK;
}

//
// TABLE/ARRAY OPERATIONS
//

TYC_RESULT tyc_geti(TycheVM* T, int index, size_t n)
{
    TYC_RESULT r;
    VALUE v_array;
    Array* array;

    TRY(stack_at(T->stack, index, &v_array))
    TRY(heap_get_array(T->heap, value_heap_key(v_array), &array))
    VALUE output = array_get(array, n);
    TRY(stack_push(T->stack, output))

    return T_OK;
}

TYC_RESULT tyc_seti(TycheVM* T, int index, size_t n)
{
    TYC_RESULT r;
    VALUE v_array, item;
    Array* array;

    TRY(stack_at(T->stack, index, &v_array))
    TRY(stack_pop(T->stack, &item))
    TRY(heap_get_array(T->heap, value_heap_key(v_array), &array))
    array_set(array, n, item);

    return T_OK;
}

TYC_RESULT tyc_getkv(TycheVM* T, int index)
{
    TYC_RESULT r;
    VALUE v_table, v_key, output;
    Table* table;

    TRY(stack_at(T->stack, index, &v_table))
    TRY(heap_get_table(T->heap, value_heap_key(v_table), &table))
    TRY(stack_pop(T->stack, &v_key))
    TRY(table_get(table, v_key, &output))
    TRY(stack_push(T->stack, output))

    return T_OK;
}

TYC_RESULT tyc_setkv(TycheVM* T, int index)
{
    TYC_RESULT r;
    VALUE v_table, v_key, v_value;
    Table* table;

    TRY(stack_at(T->stack, index, &v_table))
    TRY(heap_get_table(T->heap, value_heap_key(v_table), &table))
    TRY(stack_pop(T->stack, &v_value))
    TRY(stack_pop(T->stack, &v_key))
    table_set(table, v_key, v_value);

    return T_OK;
}

TYC_RESULT tyc_append(TycheVM* T, int index)
{
    TYC_RESULT r;
    VALUE v_array, item;
    Array* array;

    TRY(stack_at(T->stack, index, &v_array))
    TRY(stack_pop(T->stack, &item))
    TRY(heap_get_array(T->heap, value_heap_key(v_array), &array))
    array_append(array, item);

    return T_OK;
}

TYC_RESULT tyc_next(TycheVM* T, int index)
{
    TYC_RESULT r;
    VALUE obj;
    TRY(stack_at(T->stack, index, &obj))

    TYC_TYPE type = value_type(obj);

    if (type == TT_ARRAY) {
        // find array
        Array* array;
        TRY(heap_get_array(T->heap, value_heap_key(obj), &array))

        // find next key
        VALUE current_idx_value;
        TRY(stack_pop(T->stack, &current_idx_value))
        int current_idx = 0;
        if (value_type(current_idx_value) == TT_INTEGER) {
            current_idx = value_integer(current_idx_value) + 1;
        } else if (value_type(current_idx_value) != TT_NIL) {
            ERROR("Expected nil or integer key")
        }

        // add information to stack
        VALUE value = array_get(array, (size_t) current_idx);
        if (value_type(value) == TT_NIL) {
            TRY(stack_push(T->stack, create_value_nil()))
        } else {
            TRY(stack_push(T->stack, create_value_integer(current_idx)))
            TRY(stack_push(T->stack, value))
        }

    } else if (type == TT_TABLE) {
        // find array
        Table* table;
        TRY(heap_get_table(T->heap, value_heap_key(obj), &table))

        VALUE current_key, new_key, new_value;
        TRY(stack_pop(T->stack, &current_key))
        if (table_next(table, current_key, &new_key, &new_value)) {
            TRY(stack_push(T->stack, new_key))
            TRY(stack_push(T->stack, new_value))
        } else {
            TRY(stack_push(T->stack, create_value_nil()))
        }

    } else {
        ERROR("Expected array or table")
    }

    return T_OK;
}

TYC_RESULT tyc_setsupertable(TycheVM* T, int index)
{
    TYC_RESULT r;

    VALUE childtable_v;
    TRY(stack_at(T->stack, index, &childtable_v))
    if (value_type(childtable_v) != TT_TABLE)
        ERROR("Can only set supertable of a table.")

    VALUE supertable_v;
    TRY(stack_pop(T->stack, &supertable_v))
    if (value_type(supertable_v) == TT_NIL) {
        TRY(heap_remove_supertable(T->heap, value_heap_key(childtable_v)))
    } else if (value_type(supertable_v) != TT_TABLE) {
        ERROR("Supertable must be a table")
    } else {
        TRY(heap_set_supertable(T->heap, value_heap_key(childtable_v), value_heap_key(supertable_v)))
    }

    return T_OK;
}

//
// MEMORY OPERATION
//

TYC_RESULT tyc_gc(TycheVM* T)
{
    VALUE* v_idx;
    stack_push(T->stack, T->global_table);  // add global table to stack temporarily for GC
    size_t v_sz = stack_collectable_array(T->stack, &v_idx);
    heap_gc(T->heap, v_idx, v_sz);
    stack_pop(T->stack, NULL);
    free(v_idx);
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
            if (inst.operand < 0 || inst.operand >= (int) code_n_functions(T->code))
                ERROR("Value out of range")
            TRY(stack_push(T->stack, create_value_function_idx((uint32_t) inst.operand)))
            break;

        case TO_PUSHC:
            if (inst.operand < 0 || inst.operand >= (int) code_n_consts(T->code))
                ERROR("Value out of range")
            if (code_const_type(T->code, (size_t) inst.operand) == TC_STRING) {
                const char* string = code_const_string(T->code, (uint32_t) inst.operand);
                HEAP_KEY key = heap_add_string(T->heap, string, true);
                TRY(stack_push(T->stack, create_value_heap_key(TT_STRING, key)))
            } else {
                T_REAL real = code_const_real(T->code, (uint32_t) inst.operand);
                TRY(stack_push(T->stack, create_value_real(real)))
            }
            break;

        case TO_PUSHT:
            tyc_pushboolean(T, true);
            break;

        case TO_PUSHZ:
            tyc_pushboolean(T, false);
            break;

        case TO_POP:
            TRY(stack_pop(T->stack, NULL))
            break;

        case TO_NEWA:
            TRY(tyc_newarray(T))
            break;

        case TO_NEWT:
            TRY(tyc_newtable(T))
            break;

        //
        // function calls
        //

        case TO_CALL:
            if (inst.operand < 0)
                ERROR("Value out of range")
            enter_function(T, (uint16_t) inst.operand);
            break;

        case TO_RET:
            TRY(stack_pop(T->stack, &a))
            TRY(stack_pop_fp(T->stack))
            TRY(stack_push(T->stack, a))
            location_pop(T);
            goto dont_update_pc;

        //
        // table/array operations
        //

        case TO_GETI:
            if (inst.operand < 0)
                ERROR("Value out of range")
            TRY(tyc_geti(T, -1, (size_t) inst.operand))
            break;

        case TO_SETI:
            if (inst.operand < 0)
                ERROR("Value out of range")
            TRY(tyc_seti(T, -2, (size_t) inst.operand))
            break;

        case TO_APPND:
            TRY(tyc_append(T, -2))
            break;

        case TO_GETKV:
            TRY(tyc_getkv(T, -2))
            break;

        case TO_SETKV:
            TRY(tyc_setkv(T, -3))
            break;

        case TO_NEXT:
            TRY(tyc_next(T, -2))
            break;

        case TO_SPTB:
            TRY(tyc_setsupertable(T, -2))
            break;

        //
        // variables
        //

        case TO_PUSHV:
            if (inst.operand <= 0)
                ERROR("Value out of range")
            for (int i = 0; i < inst.operand; ++i)
                tyc_pushnil(T);
            break;

        case TO_SET:
            if (inst.operand < 0)
                ERROR("Value out of range")
            TRY(stack_pop(T->stack, &a))
            TRY(stack_set(T->stack, inst.operand, a))
            break;

        case TO_DUPV:
            if (inst.operand < 0)
                ERROR("Value out of range")
            TRY(stack_at(T->stack, inst.operand, &a))
            stack_push(T->stack, a);
            break;

        case TO_GLBL:
            TRY(stack_push(T->stack, T->global_table))
            break;

        //
        // expressions
        //

        case TO_SUM:  TRY(tyc_expr(T, TX_SUM));  break;
        case TO_SUB:  TRY(tyc_expr(T, TX_SUB));  break;
        case TO_MUL:  TRY(tyc_expr(T, TX_MUL));  break;
        case TO_DIV:  TRY(tyc_expr(T, TX_DIV)); break;
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
        case TO_NOT:  TRY(tyc_expr(T, TX_NOT));  break;
        case TO_NEG:  TRY(tyc_expr(T, TX_NEG));  break;
        case TO_LEN:  TRY(tyc_expr(T, TX_LEN));  break;

        //
        // jumps/branching
        //

        case TO_JMP:
            if (inst.operand < 0 || inst.operand >= (int) code_function_sz(T->code, loc->function_id))
                ERROR("Value out of range")
            loc->pc = (uint32_t) inst.operand;
            goto dont_update_pc;

        case TO_BZ:
            if (inst.operand < 0 || inst.operand >= (int) code_function_sz(T->code, loc->function_id))
                ERROR("Value out of range")
            TRY(stack_peek(T->stack, &a))
            if (value_is_zero(a)) {
                loc->pc = (uint32_t) inst.operand;
                goto dont_update_pc;
            }
            break;

        case TO_BNIL:
            if (inst.operand < 0 || inst.operand >= (int) code_function_sz(T->code, loc->function_id))
                ERROR("Value out of range")
            TRY(stack_peek(T->stack, &a))
            if (value_type(a) == TT_NIL) {
                loc->pc = (uint32_t) inst.operand;
                goto dont_update_pc;
            }
            break;

        case TO_BNZ:
            if (inst.operand < 0 || inst.operand >= (int) code_function_sz(T->code, loc->function_id))
                ERROR("Value out of range")
            TRY(stack_peek(T->stack, &a))
            if (!value_is_zero(a)) {
                loc->pc = (uint32_t) inst.operand;
                goto dont_update_pc;
            }
            break;

        //
        // memory management
        //

        case TO_GC:
            tyc_gc(T);
            break;

        default:
            ERROR("Invalid opcode 0x%x", inst.operator)
    }

    loc->pc += inst.sz;

dont_update_pc:
#ifdef DEBUG_ASSEMBLY
    debug_stack(T);
#endif
    return T_OK;
}
