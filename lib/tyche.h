#ifndef TYCHE_TYCHE_H
#define TYCHE_TYCHE_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    TT_NIL, TT_INTEGER, TT_REAL, TT_STRING, TT_STRING_CONST, TT_ARRAY, TT_TABLE, TT_FUNCTION, TT_NATIVE_PTR,
    TT_COUNT__
} TYC_TYPE;

typedef enum {
    T_OK = 0,
    T_ERR_STACK_UNDERFLOW = -1, T_ERR_STACK_FP_UNDERFLOW = -2, T_ERR_STACK_ACCESS_OUT_OF_RANGE = -3,
    T_ERR_HEAP_KEY_NOT_FOUND = -10,
    T_ERR_TABLE_KEY_NOT_FOUND = -20,
    T_ERR_ASSEMBLER_SYNTAX_ERROR = -30,
    T_ERR_BYTECODE_TOO_SMALL = -40, T_ERR_BYTECODE_INVALID_MAGIC = -41,
    T_ERR_TYPE_UNEXPECTED = -50, T_ERR_INVALID_OPCODE = -51, T_ERR_EXPR_INCORRECT_TYPES = -52, T_ERR_VALUE_OUT_OF_RANGE = -53,
} TYC_RESULT;

typedef enum {
    TX_SUM, TX_SUB, TX_MUL, TX_IDIV, TX_EQ, TX_NEQ, TX_LT, TX_LTE, TX_GT, TX_GTE, TX_AND, TX_OR, TX_XOR, TX_POW,
    TX_SHL, TX_SHR, TX_MOD,
    TX_COUNT__
} TYC_EXPR;

#define T_REAL float

typedef struct TycheVM TycheVM;

// create/destroy VM
TycheVM*   tyc_new(void);
void       tyc_destroy(TycheVM* t);

// code loading and execution
TYC_RESULT tyc_load_bytecode(TycheVM* T, uint8_t const* bytecode, size_t bytecode_sz);
TYC_RESULT tyc_call(TycheVM* t, uint16_t n_pars);

// stack manipulation and query
size_t     tyc_stack_size(TycheVM* T);
void       tyc_pushnil(TycheVM* T);
void       tyc_pushinteger(TycheVM* T, int32_t value);
TYC_RESULT tyc_type(TycheVM* T, int idx, TYC_TYPE* type);
TYC_RESULT tyc_tointeger(TycheVM* T, int idx, int32_t* value);
TYC_RESULT tyc_expr(TycheVM* T, TYC_EXPR expr);

#endif //TYCHE_TYCHE_H
