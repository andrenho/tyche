#ifndef TYCHE_PRIV_H
#define TYCHE_PRIV_H

#include "tyche.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

//
// INSTRUCTIONS
//

typedef enum {
    // STACK OPERATIONS
    TO_PUSHI = 0XA0,
    TO_PUSHC = 0XA1,
    TO_PUSHF = 0XA2,
    TO_PUSHN = 0X00,
    TO_PUSHZ = 0X01,
    TO_PUSHT = 0X02,
    TO_NEWA  = 0X03,
    TO_NEWT  = 0X04,
    TO_POP   = 0X05,
    TO_DUP   = 0X06,

    // LOCAL VARIABLES
    TO_PUSHV = 0XA3,
    TO_SET   = 0XAE,
    TO_DUPV  = 0XA4,
    TO_SETG  = 0XA5,
    TO_GETG  = 0XA6,

    // FUNCTION OPERATIONS
    TO_CALL = 0XA7,
    TO_RET  = 0X10,
    TO_RETI = 0X11,

    // TABLE AND ARRAY OPERATIONS
    TO_GETKV = 0X16,
    TO_SETKV = 0X17,
    TO_GETI  = 0XA8,
    TO_SETI  = 0XA9,
    TO_APPND = 0X18,
    TO_NEXT  = 0X19,
    TO_SMT   = 0X1A,
    TO_MT    = 0X1B,

    // LOGICAL/ARITHMETIC
    TO_SUM     = 0X20,
    TO_SUB     = 0X21,
    TO_MUL     = 0X22,
    TO_DIV     = 0X23,
    TO_IDIV    = 0X24,
    TO_MOD     = 0X25,
    TO_EQ      = 0X26,
    TO_NEQ     = 0X27,
    TO_LT      = 0X28,
    TO_LTE     = 0X29,
    TO_GT      = 0X2A,
    TO_GTE     = 0X2B,
    TO_AND     = 0X2C,
    TO_OR      = 0X2D,
    TO_XOR     = 0X2E,
    TO_POW     = 0X2F,
    TO_SHL     = 0X30,
    TO_SHR     = 0X31,

    // OTHER VALUE OPERATIONS
    TO_LEN  = 0X40,
    TO_TYPE = 0X41,
    TO_CAST = 0XAD,
    TO_VER  = 0X42,

    // EXTERNAL CODE
    TO_CMPL  = 0X48,
    TO_ASMBL = 0X49,
    TO_LOAD  = 0X4A,

    // CONTROL FLOW
    TO_BZ   = 0XAA,
    TO_BNZ  = 0XAB,
    TO_BNIL = 0xAF,
    TO_JMP  = 0XAC,

    // MEMORY MANAGEMENT
    TO_GC = 0X4B,
} TYC_INST;

//
// ERROR MANAGEMENT
//

extern __thread char last_err_msg[256];
extern bool abort_on_errors;   // only on debug mode

#ifdef DEBUG_ASSEMBLY
#define ERROR(...) {                                            \
    snprintf(last_err_msg, sizeof last_err_msg, __VA_ARGS__);   \
    if (abort_on_errors) {                                      \
        fprintf(stderr, "%s\n", last_err_msg);                  \
        abort();                                                \
    }                                                           \
    return T_ERR;                                               \
}
#else
#define ERROR(...) {                                            \
    snprintf(last_err_msg, sizeof last_err_msg, __VA_ARGS__);   \
    fprintf(stderr, "%s\n", last_err_msg);                      \
    return T_ERR;                                               \
}
#endif

#define TRY(x) if ((r = (x)) != T_OK) { return r; }


//
// TYPE DECLARATION
//

typedef uint32_t HEAP_KEY;

typedef struct {
    TYC_TYPE type;
    union {
        int32_t  i;
        float    f;
        uint32_t idx;
        HEAP_KEY heap_key;
    } v;
} VALUE;

typedef struct Stack   Stack;
typedef struct Array   Array;
typedef struct Table   Table;
typedef struct Heap    Heap;
typedef struct Code    Code;
typedef struct Strings Strings;

typedef uint64_t TABLE_HASH;

typedef enum {
    TC_STRING, TC_REAL, TC_INVALID_TYPE
} TYC_CONST_TYPE;

typedef struct Instruction {
    TYC_INST operator;
    int32_t  operand;
    uint8_t  sz;
} Instruction;

//
// UTILS
//

__attribute__((noreturn)) void out_of_memory(void);
void* xmalloc(size_t n);
void* xcalloc(size_t n, size_t size);
void* xrealloc(void* p, size_t n);

//
// VALUE
//

TYC_TYPE    value_type(VALUE v);

const char* type_name(TYC_TYPE t);
bool        type_is_collectable(TYC_TYPE t);

int32_t  value_integer(VALUE v);
float    value_real(VALUE v);
uint32_t value_idx(VALUE v);
HEAP_KEY value_heap_key(VALUE v);
bool     value_is_zero(VALUE v);

VALUE create_value_nil(void);
VALUE create_value_from_bool(bool b);
VALUE create_value_integer(int32_t v);
VALUE create_value_real(float f);
VALUE create_value_idx(TYC_TYPE type, uint32_t idx);
VALUE create_value_heap_key(TYC_TYPE type, HEAP_KEY key);

//
// STACK
//

Stack*     stack_new(void);
void       stack_destroy(Stack* s);

TYC_RESULT stack_push(Stack* s, VALUE v);
TYC_RESULT stack_peek(Stack const* s, VALUE* v_out);
TYC_RESULT stack_pop(Stack* s, VALUE* v_out);

size_t     stack_size(Stack const* s);

TYC_RESULT stack_at(Stack const* s, int32_t pos, VALUE* v);
TYC_RESULT stack_set(Stack* s, int32_t pos, VALUE v);

size_t     stack_top_fp(Stack const* s);
TYC_RESULT stack_push_fp(Stack* s);
TYC_RESULT stack_pop_fp(Stack* s);
size_t     stack_fp_level(Stack const* s);

size_t     stack_collectable_array(Stack const* s, VALUE** values);

//
// HEAP ARRAY
//

Array* array_new(void);
void   array_destroy(Array* a);

size_t array_len(Array const* a);
VALUE  array_get(Array const* a, size_t pos);
void   array_set(Array* a, size_t pos, VALUE v);
void   array_append(Array* a, VALUE v);

//
// HEAP TABLE
//

Table*     table_new(Heap const* heap);
void       table_destroy(Table* t);

size_t     table_len(Table* t);
TYC_RESULT table_get(Table const* t, VALUE key, VALUE* value);
void       table_set(Table* t, VALUE key, VALUE value);
void       table_del(Table* t, VALUE key);

void       table_foreach(Table* t, void(f)(VALUE key, VALUE value, void* data), void* data);
size_t     table_size(Table* t);

bool       table_next(Table* t, VALUE key, VALUE* out_key, VALUE* out_value);

//
// HEAP
//

Heap*      heap_new(void);
void       heap_destroy(Heap* h);

HEAP_KEY   heap_add_string(Heap* h, const char* value, bool constant);
TYC_RESULT heap_get_string(Heap const* h, HEAP_KEY key, const char** value);

HEAP_KEY   heap_add_array(Heap* h);
TYC_RESULT heap_get_array(Heap const* h, HEAP_KEY key, Array** array);

HEAP_KEY   heap_add_table(Heap* h);
TYC_RESULT heap_get_table(Heap const* h, HEAP_KEY key, Table** table);

size_t     heap_size(Heap const* h);

void       heap_gc(Heap* h, VALUE const* roots, size_t n_roots);

//
// CODE
//

TYC_RESULT     code_assemble(const char* code, uint8_t** bytecode, size_t* bytecode_sz);

Code*          code_new(void);
void           code_destroy(Code* code);

TYC_RESULT     code_load_bytecode(Code* code, uint8_t const* bytecode, size_t bytecode_sz);

uint32_t       code_n_consts(Code const* code);
TYC_CONST_TYPE code_const_type(Code const* code, size_t n);

T_REAL         code_const_real(Code const* code, size_t n);
const char*    code_const_string(Code const* code, size_t n);

uint32_t       code_n_functions(Code const* code);
uint32_t       code_function_sz(Code const* code, uint32_t f_id);
Instruction    code_next_instruction(Code const* code, uint32_t function_id, uint32_t pc);

void           code_debug_bytecode(Code const* code);
void           code_decompile(Code const* code);
void           code_parse_instruction(Instruction inst, char* outbuf, size_t sz);

//
// VM PRIVATE METHODS
//

Stack* tyc_stack(TycheVM* T);
Heap*  tyc_heap(TycheVM* T);
Code*  tyc_code(TycheVM* T);

//
// EXPRESSIONS
//

void       expr_init(void);
TYC_RESULT binary_expr(TycheVM* T, TYC_EXPR op, VALUE a, VALUE b, VALUE* result);

#endif //TYCHE_PRIV_H
