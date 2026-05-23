#ifndef TYCHE_TYCHE_H
#define TYCHE_TYCHE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    TT_NIL, TT_BOOLEAN, TT_INTEGER, TT_REAL, TT_NATIVE_PTR, TT_STRING, TT_ARRAY, TT_TABLE, TT_FUNCTION,

    // internal types
    TT_COUNT__
} TYC_TYPE;

typedef enum {
    T_OK = 0, T_ERR = -1,
} TYC_RESULT;

typedef enum {
    TX_SUM, TX_SUB, TX_MUL, TX_IDIV, TX_EQ, TX_NEQ, TX_LT, TX_LTE, TX_GT, TX_GTE, TX_AND, TX_OR, TX_XOR, TX_POW,
    TX_SHL, TX_SHR, TX_MOD,
    TX_COUNT__
} TYC_EXPR;

#define T_REAL double

typedef struct TycheVM TycheVM;

// error management
const char* tyc_last_error(void);

// create/destroy VM
TycheVM*   tyc_new(void);
void       tyc_destroy(TycheVM* T);

// debugging (DEBUG_ASSEMBLY needs to be setup in compilation options)
void       tyc_debug_to_console(TycheVM* T, bool activate);
void       tyc_assembly_decompile(TycheVM* T);
void       tyc_print_bytecode(TycheVM* T);

// code loading and execution
TYC_RESULT tyc_load_bytecode(TycheVM* T, uint8_t const* bytecode, size_t bytecode_sz);
TYC_RESULT tyc_call(TycheVM* t, uint16_t n_pars);

// stack insertions
TYC_RESULT tyc_pushnil(TycheVM* T);
TYC_RESULT tyc_pushinteger(TycheVM* T, int32_t value);
TYC_RESULT tyc_pushstring(TycheVM* T, const char* value);
TYC_RESULT tyc_newarray(TycheVM* T);
TYC_RESULT tyc_newtable(TycheVM* T);

// stack query
size_t     tyc_stack_size(TycheVM* T);
TYC_RESULT tyc_type(TycheVM* T, int idx, TYC_TYPE* type);
TYC_RESULT tyc_toboolean(TycheVM* T, int idx, bool* value);
TYC_RESULT tyc_tointeger(TycheVM* T, int idx, int32_t* value);
TYC_RESULT tyc_tostring(TycheVM* T, int idx, const char** str);

// table/array operations
TYC_RESULT tyc_geti(TycheVM* T, int index, size_t n);
TYC_RESULT tyc_seti(TycheVM* T, int index, size_t n);
TYC_RESULT tyc_append(TycheVM* T, int index);
TYC_RESULT tyc_next(TycheVM* T, int index);
TYC_RESULT tyc_setkv(TycheVM* T, int index);
TYC_RESULT tyc_getkv(TycheVM* T, int index);
TYC_RESULT tyc_setsupertable(TycheVM* T, int index);

// memory operations
TYC_RESULT tyc_gc(TycheVM* T);

TYC_RESULT tyc_expr(TycheVM* T, TYC_EXPR expr);

#endif //TYCHE_TYCHE_H
