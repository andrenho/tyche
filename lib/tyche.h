#ifndef TYCHE_TYCHE_H
#define TYCHE_TYCHE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    TYC_NIL          = 0,
    TYC_BOOLEAN      = 1,
    TYC_INTEGER      = 2,
    TYC_REAL         = 3,
    TYC_STRING       = 4,
    TYC_ARRAY        = 5,
    TYC_TABLE        = 6,
    TYC_FUNCTION     = 7,
    TYC_NATIVE_PTR   = 8,

    // internal types
    TYC_NATIVE_FN__  = 9,
    TYC_COUNT__
} TYC_TYPE;

typedef enum {
    TYC_OK = 0, TYC_ERR = 1,
} TYC_RESULT;

typedef enum {
    TX_SUM, TX_SUB, TX_MUL, TX_DIV, TX_IDIV, TX_EQ, TX_NEQ, TX_LT, TX_LTE, TX_GT, TX_GTE, TX_AND, TX_OR, TX_XOR, TX_POW,
    TX_SHL, TX_SHR, TX_MOD, TX_NOT, TX_NEG, TX_HASH,
    TX_COUNT__
} TYC_EXPR;

#define TYCHE_REAL double

typedef struct TycheVM TycheVM;

// error management
const char* tyc_last_error(void);

// create/destroy VM
TycheVM*   tyc_new(void);
void       tyc_destroy(TycheVM* T);

// debugging
void       tyc_debug_to_console(TycheVM* T, bool activate);
void       tyc_assembly_decompile(TycheVM* T);
void       tyc_print_bytecode(TycheVM* T);

// code loading and execution
TYC_RESULT tyc_assemble(const char* asm_src, uint8_t** bytecode, size_t* bytecode_sz);
TYC_RESULT tyc_load_assembly(TycheVM* T, const char* asm_src);
TYC_RESULT tyc_load_bytecode(TycheVM* T, uint8_t const* bytecode, size_t bytecode_sz);
TYC_RESULT tyc_call(TycheVM* T, uint16_t n_pars);

// stack insertions
TYC_RESULT tyc_pushnil(TycheVM* T);
TYC_RESULT tyc_pushboolean(TycheVM* T, bool value);
TYC_RESULT tyc_pushinteger(TycheVM* T, int32_t value);
TYC_RESULT tyc_pushreal(TycheVM* T, TYCHE_REAL value);
TYC_RESULT tyc_pushstring(TycheVM* T, const char* value);
TYC_RESULT tyc_pushnativeptr(TycheVM* T, void* ptr);
TYC_RESULT tyc_pushnativefunction(TycheVM* T, TYC_RESULT(*f)(TycheVM*));
TYC_RESULT tyc_newarray(TycheVM* T);
TYC_RESULT tyc_newtable(TycheVM* T);
TYC_RESULT tyc_dup(TycheVM* T, int idx);
TYC_RESULT tyc_pop(TycheVM* T);

// stack query
size_t     tyc_stack_size(TycheVM* T);
TYC_RESULT tyc_type(TycheVM* T, int idx, TYC_TYPE* type);
TYC_RESULT tyc_toboolean(TycheVM* T, int idx, bool* value);
TYC_RESULT tyc_tointeger(TycheVM* T, int idx, int32_t* value);
TYC_RESULT tyc_toreal(TycheVM* T, int idx, TYCHE_REAL* value);
TYC_RESULT tyc_tostring(TycheVM* T, int idx, const char** str);
TYC_RESULT tyc_tonativeptr(TycheVM* T, int idx, void** ptr);

// table/array operations
TYC_RESULT tyc_geti(TycheVM* T, int idx, size_t n);
TYC_RESULT tyc_seti(TycheVM* T, int idx, size_t n);
TYC_RESULT tyc_append(TycheVM* T, int idx);
TYC_RESULT tyc_next(TycheVM* T, int idx);
TYC_RESULT tyc_setkv(TycheVM* T, int idx);
TYC_RESULT tyc_getkv(TycheVM* T, int idx);
TYC_RESULT tyc_setoper(TycheVM* T, TYC_EXPR oper);
TYC_RESULT tyc_setsupertable(TycheVM* T, int idx);

TYC_RESULT tyc_global(TycheVM* T);

// memory operations
TYC_RESULT tyc_gc(TycheVM* T);

// expressions
TYC_RESULT tyc_expr(TycheVM* T, TYC_EXPR expr);
TYC_RESULT tyc_len(TycheVM* T, int idx);

// exceptions
TYC_RESULT tyc_throw(TycheVM* T, const char* message);

#endif //TYCHE_TYCHE_H
