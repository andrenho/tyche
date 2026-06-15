#ifndef TYCHE_PRIV_H
#define TYCHE_PRIV_H

#include "tyche.h"

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "nanbox.h"
#include "instructions/instructions.h"

//
// ERROR MANAGEMENT
//

extern __thread char last_err_msg[256];
extern bool abort_on_errors;   // only on debug mode

#define EPSILON 0.000000000001

#ifdef DEBUG_ASSEMBLY
#define ERROR(...) {                                            \
    snprintf(last_err_msg, sizeof last_err_msg, __VA_ARGS__);   \
    if (abort_on_errors) {                                      \
        fprintf(stderr, "%s\n", last_err_msg);                  \
        abort();                                                \
    }                                                           \
    return TYC_ERR;                                               \
}
#else
#define ERROR(...) {                                            \
    snprintf(last_err_msg, sizeof last_err_msg, __VA_ARGS__);   \
    fprintf(stderr, "%s\n", last_err_msg);                      \
    return TYC_ERR;                                               \
}
#endif

#define TRY(x) if ((r = (x)) != TYC_OK) { return r; }


//
// TYPE DECLARATION
//

typedef uint32_t HEAP_KEY;

typedef nanbox_t VALUE;

typedef TYC_RESULT(*TYCHE_CB)(TycheVM* T);

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
    TYC_INST operation;
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

bool       value_boolean(VALUE v);
int32_t    value_integer(VALUE v);
TYCHE_REAL value_real(VALUE v);
uint32_t   value_function_idx(VALUE v);
HEAP_KEY   value_heap_key(VALUE v);
bool       value_is_false(VALUE v);
bool       value_is_nil(VALUE v);
bool       value_is_tombstone(VALUE v);
void*      value_native_pointer(VALUE v);

VALUE create_value_nil(void);
VALUE create_value_bool(bool b);
VALUE create_value_integer(int32_t v);
VALUE create_value_real(TYCHE_REAL f);
VALUE create_value_function_idx(uint32_t idx);
VALUE create_value_heap_key(TYC_TYPE type, HEAP_KEY key);
VALUE create_value_native_pointer(void* ptr);
VALUE create_value_tombstone();

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

Table*  table_new(TycheVM const* T);
void    table_destroy(Table* t);

size_t  table_len(Table const* t);
bool    table_get(Table const* t, VALUE key, VALUE* value);
void    table_set(Table* t, VALUE key, VALUE value);
void    table_del(Table* t, VALUE key);

bool    table_has_key(Table const* t, VALUE key);

bool    table_next(Table const* t, VALUE key, VALUE* out_key, VALUE* out_value);
void    table_setsuper(Table* t, Table* super);

void    table_debug_internals(Table const* t);

//
// HEAP
//

Heap*      heap_new(void);
void       heap_destroy(Heap* h);

HEAP_KEY   heap_add_string(Heap* h, const char* value, bool constant);
TYC_RESULT heap_get_string(Heap const* h, HEAP_KEY key, const char** value);

HEAP_KEY   heap_add_array(Heap* h);
TYC_RESULT heap_get_array(Heap const* h, HEAP_KEY key, Array** array);

HEAP_KEY   heap_add_table(Heap* h, TycheVM const* T);
TYC_RESULT heap_get_table(Heap const* h, HEAP_KEY key, Table** table);
TYC_RESULT heap_set_supertable(Heap const* h, HEAP_KEY table, HEAP_KEY super);
TYC_RESULT heap_remove_supertable(Heap const* h, HEAP_KEY table);

HEAP_KEY   heap_add_native_function(Heap* h, TYCHE_CB cb);
TYC_RESULT heap_get_native_function(Heap const* h, HEAP_KEY key, TYCHE_CB* cb);

size_t     heap_size(Heap const* h);

bool       heap_should_gc(Heap* h);
void       heap_gc(Heap* h, VALUE const* roots, size_t n_roots);

void       heap_debug(Heap* h);

//
// CODE
//

#define MAGIC 0xa7d6e9b1

TYC_RESULT     code_assemble(const char* code, uint8_t** bytecode, size_t* bytecode_sz);

Code*          code_new(void);
void           code_destroy(Code* code);

TYC_RESULT     code_load_bytecode(Code* code, uint8_t const* bytecode, size_t bytecode_sz);

uint32_t       code_n_consts(Code const* code);
TYC_CONST_TYPE code_const_type(Code const* code, size_t n);

TYCHE_REAL     code_const_real(Code const* code, size_t n);
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

#define NATIVE_FUNCTION_ID UINT32_MAX

Stack* tyc_stack(TycheVM* T);
Heap*  tyc_heap(TycheVM* T);
Code*  tyc_code(TycheVM* T);

uint32_t tyc_hash(TycheVM const* T, VALUE value);
bool     tyc_eq(TycheVM const* T, VALUE value_a, VALUE value_b);
void     tyc_debug_value(TycheVM const* T, VALUE a);

//
// EXPRESSIONS
//

void       expr_init(void);
bool       expr_is_binary(TYC_EXPR op);
TYC_RESULT unary_expr(TycheVM* T, TYC_EXPR op, VALUE a, VALUE* result);
TYC_RESULT binary_expr(TycheVM* T, TYC_EXPR op, VALUE a, VALUE b, VALUE* result);

#endif //TYCHE_PRIV_H
