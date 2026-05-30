#include "priv.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Location {
    uint32_t function_id;
    uint32_t pc;
} Location;

typedef struct LocationStack {
    Location* locations;
    size_t    sz;
    size_t    cap;
} LocationStack;

typedef struct ErrorHandlerLocation {
    size_t   location_level;
    uint32_t pc;
} ErrorHandlerLocation;

typedef struct ErrorHandlerStack {
    ErrorHandlerLocation* locations;
    size_t                sz;
    size_t                cap;
} ErrorHandlerStack;

struct TycheVM {
    Stack*            stack;
    Heap*             heap;
    Code*             code;
    LocationStack     location_stack;
    ErrorHandlerStack error_stack;
    VALUE             global_table;
    bool              debug;
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
    T->error_stack = (ErrorHandlerStack) {
            .locations = xmalloc(4 * sizeof(ErrorHandlerLocation)),
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

    free(T->error_stack.locations);
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
            printf("<ptr %p>", (void *) (uintptr_t) value_function_idx(a));
            break;
        case TT_NATIVE_FN: {
            TYCHE_CB fn;
            if (heap_get_native_function(T->heap, value_heap_key(a), &fn))
                printf("<ptr-fn %p>", (void *) (uintptr_t) fn);
            else
                printf("{(fn - not implemented )}\n");
            break;
        }
        case TT_COUNT__:
        default:
            __builtin_unreachable();
    }
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

static TYC_RESULT enter_function(TycheVM* T, uint16_t n_pars, bool* is_native, VALUE* function)
{
    TYC_RESULT r;

    // get parameters
    VALUE* params = xcalloc(n_pars + 1, sizeof(VALUE));
    for (uint16_t i = 0; i < n_pars; ++i)
        TRY(stack_pop(T->stack, &params[i]))

    // get function
    TRY(stack_pop(T->stack, function))
    if (value_type(*function) != TT_FUNCTION && value_type(*function) != TT_NATIVE_FN)
        ERROR("Expected function")

    // push FP
    stack_push_fp(T->stack);

    // pass parameters
    for (int i = n_pars-1; i >= 0; --i)
        TRY(stack_push(T->stack, params[i]))
    free(params);

    // prepare to run function
    if (value_type(*function) == TT_FUNCTION) {
        *is_native = false;
        push_location(T, value_function_idx(*function), 0);
    } else if (value_type(*function) == TT_NATIVE_FN) {
        *is_native = true;
        push_location(T, NATIVE_FUNCTION_ID, 0);
    }

    return T_OK;
}

static TYC_RESULT exit_function(TycheVM* T)
{
    TYC_RESULT r;
    VALUE a;
    TRY(stack_pop(T->stack, &a))
    TRY(stack_pop_fp(T->stack))
    TRY(stack_push(T->stack, a))
    location_pop(T);
    return T_OK;
}

static TYC_RESULT run_until_return(TycheVM* T)
{
    TYC_RESULT r;

    while (T->location_stack.sz > 0)
        TRY(step(T));

    return T_OK;
}

static TYC_RESULT run_native_function(TycheVM* T, VALUE function)
{
    if (value_type(function) != TT_NATIVE_FN)
        abort();

    TYC_RESULT r;
    TYCHE_CB f;
    TRY(heap_get_native_function(T->heap, value_heap_key(function), &f))
    f(T);
    exit_function(T);

    return T_OK;
}

TYC_RESULT tyc_call(TycheVM* T, uint16_t n_pars)
{
    TYC_RESULT r;
    bool is_native;
    VALUE f;
    TRY(enter_function(T, n_pars, &is_native, &f))
    if (is_native) {
        TRY(run_native_function(T, f))
    } else {
        TRY(run_until_return(T))
    }
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

TYC_RESULT tyc_pushnativeptr(TycheVM* T, void* ptr)
{
    return stack_push(T->stack, create_value_native_pointer(ptr));
}

TYC_RESULT tyc_pushnativefunction(TycheVM* T, void(*f)(TycheVM*))
{
    return stack_push(T->stack, create_value_heap_key(TT_NATIVE_FN, heap_add_native_function(T->heap, f)));
}

TYC_RESULT tyc_pop(TycheVM* T)
{
    return stack_pop(T->stack, NULL);
}

TYC_RESULT tyc_newarray(TycheVM* T)
{
    return stack_push(T->stack, create_value_heap_key(TT_ARRAY, heap_add_array(T->heap)));
}

TYC_RESULT tyc_newtable(TycheVM* T)
{
    return stack_push(T->stack, create_value_heap_key(TT_TABLE, heap_add_table(T->heap)));
}

TYC_RESULT tyc_dup(TycheVM* T, int idx)
{
    TYC_RESULT r;
    VALUE a;
    TRY(stack_at(T->stack, idx, &a))
    TRY(stack_push(T->stack, a))
    return T_OK;
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
}

TYC_RESULT tyc_tonativeptr(TycheVM* T, int idx, void** ptr)
{
    VALUE v;
    TYC_RESULT r;
    TRY(stack_at(T->stack, idx, &v))
    if (value_type(v) != TT_NATIVE_PTR)
        ERROR("Expected native pointer")
    *ptr = value_native_pointer(v);
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

TYC_RESULT tyc_len(TycheVM* T, int idx)
{
    TYC_RESULT r;
    VALUE a;
    TRY(stack_at(T->stack, idx, &a))

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (value_type(a)) {
        case TT_STRING: {
            const char* str;
            heap_get_string(T->heap, value_heap_key(a), &str);
            return stack_push(T->stack, create_value_integer((int32_t) strlen(str)));
        }
        case TT_ARRAY: {
            Array* array;
            heap_get_array(T->heap, value_heap_key(a), &array);
            return stack_push(T->stack, create_value_integer((int32_t) array_len(array)));
        }
        case TT_TABLE: {
            Table* table;
            heap_get_table(T->heap, value_heap_key(a), &table);
            return stack_push(T->stack, create_value_integer((int32_t) table_len(table)));
        }
        default:;
    }
#pragma GCC diagnostic pop
    ERROR("Len not supported for type '%s'", type_name(value_type(a)))
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
// ERROR HANDLING
//

static TYC_RESULT tyc_push_errhandler(TycheVM* T, uint32_t pc)
{
    if (T->error_stack.sz == T->error_stack.cap) {
        T->error_stack.cap *= 2;
        T->error_stack.locations = xrealloc(T->error_stack.locations, T->error_stack.cap * sizeof(ErrorHandlerLocation));
    }

    T->error_stack.locations[T->error_stack.sz] = (ErrorHandlerLocation) {
            .location_level = T->location_stack.sz,
            .pc = pc,
    };
    ++T->error_stack.sz;

    return T_OK;
}

static TYC_RESULT tyc_pop_errhandler(TycheVM* T)
{
    if (T->error_stack.sz == 0)
        ERROR("Error stack: underflow");
    --T->error_stack.sz;
    return T_OK;
}

static TYC_RESULT tyc_throw_raw(TycheVM* T)
{
    TYC_RESULT r;

    if (T->error_stack.sz == 0) {  // error at toplevel
        VALUE a = create_value_nil();
        stack_peek(T->stack, &a);
        printf("\ntyche error: "); debug_value(T, a); printf("\n");
        exit(EXIT_FAILURE);
    }

    ErrorHandlerLocation* err_loc = &T->error_stack.locations[T->error_stack.sz - 1];
    while (T->location_stack.sz > err_loc->location_level)
        location_pop(T);

    T->location_stack.locations[T->location_stack.sz - 1].pc = err_loc->pc;
    TRY(tyc_pop_errhandler(T))

    return T_OK;
}

TYC_RESULT tyc_throw(TycheVM* T, const char* message)
{
    TYC_RESULT r;
    Table *t;

    HEAP_KEY k_table = heap_add_table(T->heap);
    VALUE v_table = create_value_heap_key(TT_TABLE, k_table);
    VALUE v_error = create_value_heap_key(TT_STRING, heap_add_string(T->heap, "error", false));
    VALUE v_message = create_value_heap_key(TT_STRING, heap_add_string(T->heap, message, false));
    VALUE v_function_id = create_value_heap_key(TT_STRING, heap_add_string(T->heap, "function_id", false));
    VALUE v_pc = create_value_heap_key(TT_STRING, heap_add_string(T->heap, "pc", false));

    TRY(heap_get_table(T->heap, k_table, &t))
    table_set(t, v_error, v_message);
    table_set(t, v_function_id, create_value_integer((int32_t) location_top(T)->function_id));
    table_set(t, v_pc, create_value_integer((int32_t) location_top(T)->pc));
    TRY(stack_push(T->stack, v_table))
    TRY(tyc_throw_raw(T))

    return T_OK;
}

//
// STEP
//

static TYC_RESULT step(TycheVM* T)
{
#define TRY_RUNTIME(x) if ((r = (x)) != T_OK) { goto dont_update_pc; }

    VALUE a;
    TYC_RESULT r;

    Location* loc = location_top(T);
    Instruction inst = code_next_instruction(T->code, loc->function_id, loc->pc);

#ifdef DEBUG_ASSEMBLY
    debug_instruction(T, loc, inst);
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
    switch (inst.operator) {

        //
        // stack manipulation
        //

        case TO_PUSHN:
            tyc_pushnil(T);
            break;

        case TO_PUSHI:
        case TO_PUSHI + TO_16BIT:
        case TO_PUSHI + TO_32BIT:
            tyc_pushinteger(T, inst.operand);
            break;

        case TO_PUSHF:
        case TO_PUSHF + TO_16BIT:
        case TO_PUSHF + TO_32BIT:
            if (inst.operand < 0 || inst.operand >= (int) code_n_functions(T->code))
                ERROR("Function id out of range - this is a compiler bug")
            TRY(stack_push(T->stack, create_value_function_idx((uint32_t) inst.operand)))
            break;

        case TO_PUSHC:
        case TO_PUSHC + TO_16BIT:
        case TO_PUSHC + TO_32BIT:
            if (inst.operand < 0 || inst.operand >= (int) code_n_consts(T->code))
                ERROR("Const id out of range - this is a compiler bug")
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

        case TO_DUP:
            TRY(tyc_dup(T, -1))
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
        case TO_CALL + TO_16BIT:
        case TO_CALL + TO_32BIT: {
            if (inst.operand < 0)
                ERROR("Function id out of range - this is a compiler bug")
            bool is_native;
            VALUE f;
            enter_function(T, (uint16_t) inst.operand, &is_native, &f);
            if (is_native)
                TRY(run_native_function(T, f))
            break;
        }

        case TO_RETN:
            TRY(stack_push(T->stack, create_value_nil())) // fallthrough
        case TO_RET:
            exit_function(T);
            goto dont_update_pc;

        //
        // table/array operations
        //

        case TO_GETI:
        case TO_GETI + TO_16BIT:
        case TO_GETI + TO_32BIT:
            if (inst.operand < 0) {
                tyc_throw(T, "Value out of range");
                goto dont_update_pc;
            }
            TRY(tyc_geti(T, -1, (size_t) inst.operand))
            break;

        case TO_SETI:
        case TO_SETI + TO_16BIT:
        case TO_SETI + TO_32BIT:
            if (inst.operand < 0) {
                tyc_throw(T, "Value out of range");
                goto dont_update_pc;
            }
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
        case TO_PUSHV + TO_16BIT:
        case TO_PUSHV + TO_32BIT:
            if (inst.operand <= 0)
                ERROR("Value out of range")
            for (int i = 0; i < inst.operand; ++i)
                tyc_pushnil(T);
            break;

        case TO_SET:
        case TO_SET + TO_16BIT:
        case TO_SET + TO_32BIT:
            if (inst.operand < 0)
                ERROR("Value out of range")
            TRY(stack_pop(T->stack, &a))
            TRY(stack_set(T->stack, inst.operand, a))
            break;

        case TO_DUPV:
        case TO_DUPV + TO_16BIT:
        case TO_DUPV + TO_32BIT:
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

        case TO_SUM:  TRY_RUNTIME(tyc_expr(T, TX_SUM));  break;
        case TO_SUB:  TRY_RUNTIME(tyc_expr(T, TX_SUB));  break;
        case TO_MUL:  TRY_RUNTIME(tyc_expr(T, TX_MUL));  break;
        case TO_DIV:  TRY_RUNTIME(tyc_expr(T, TX_DIV)); break;
        case TO_IDIV: TRY_RUNTIME(tyc_expr(T, TX_IDIV)); break;
        case TO_EQ:   TRY_RUNTIME(tyc_expr(T, TX_EQ));   break;
        case TO_NEQ:  TRY_RUNTIME(tyc_expr(T, TX_NEQ));  break;
        case TO_LT:   TRY_RUNTIME(tyc_expr(T, TX_LT));   break;
        case TO_LTE:  TRY_RUNTIME(tyc_expr(T, TX_LTE));  break;
        case TO_GT:   TRY_RUNTIME(tyc_expr(T, TX_GT));   break;
        case TO_GTE:  TRY_RUNTIME(tyc_expr(T, TX_GTE));  break;
        case TO_AND:  TRY_RUNTIME(tyc_expr(T, TX_AND));  break;
        case TO_OR:   TRY_RUNTIME(tyc_expr(T, TX_OR));   break;
        case TO_XOR:  TRY_RUNTIME(tyc_expr(T, TX_XOR));  break;
        case TO_POW:  TRY_RUNTIME(tyc_expr(T, TX_POW));  break;
        case TO_SHL:  TRY_RUNTIME(tyc_expr(T, TX_SHL));  break;
        case TO_SHR:  TRY_RUNTIME(tyc_expr(T, TX_SHR));  break;
        case TO_MOD:  TRY_RUNTIME(tyc_expr(T, TX_MOD));  break;
        case TO_NOT:  TRY_RUNTIME(tyc_expr(T, TX_NOT));  break;
        case TO_NEG:  TRY_RUNTIME(tyc_expr(T, TX_NEG));  break;

        case TO_LEN:
            TRY(tyc_len(T, -1));
            break;

        case TO_TYPE: {
            TYC_TYPE type;
            TRY(tyc_type(T, -1, &type))
            TRY(tyc_pushinteger(T, (int32_t) type))
            break;
        }

        case TO_VER: {
            char version[10]; snprintf(version, 10, "%d.%d", VERSION_MAJOR, VERSION_MINOR);
            TRY(tyc_pushstring(T, version))
            break;
        }

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

        //
        // error handling
        //

        case TO_PUSHE:
            if (inst.operand < 0 || inst.operand >= (int) code_function_sz(T->code, loc->function_id))
                ERROR("Value out of range")
            tyc_push_errhandler(T, (uint32_t) inst.operand);
            break;

        case TO_POPE:
            tyc_pop_errhandler(T);
            break;

        case TO_THRW:
            tyc_throw_raw(T);
            goto dont_update_pc;

        case TO_UNKNOWN:
        default:
            ERROR("Invalid opcode 0x%x", inst.operator)
    }
#pragma GCC diagnostic pop

    loc->pc += inst.sz;

dont_update_pc:
#ifdef DEBUG_ASSEMBLY
    debug_stack(T);
#endif

    if (heap_should_gc(T->heap))
        tyc_gc(T);

    return T_OK;
}
