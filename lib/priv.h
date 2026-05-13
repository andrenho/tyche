#ifndef TYCHE_PRIV_H
#define TYCHE_PRIV_H

#include "tyche.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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

typedef struct {
    TYC_TYPE type;
    union {
        int32_t  i;
        float    f;
        uint32_t idx;
    } v;
} VALUE;

TYC_TYPE value_type(VALUE v);
bool     type_is_collectable(TYC_TYPE t);

int32_t  value_integer(VALUE v);
float    value_real(VALUE v);
uint32_t value_idx(VALUE v);
bool     value_is_zero(VALUE v);

VALUE create_value_nil(void);
VALUE create_value_integer(int32_t v);
VALUE create_value_real(float f);
VALUE create_value_idx(TYC_TYPE type, uint32_t idx);

//
// STACK
//

typedef struct Stack Stack;

Stack*     stack_new(void);
void       stack_destroy(Stack* s);

TYC_RESULT stack_push(Stack* s, VALUE v);
TYC_RESULT stack_peek(Stack const* s, VALUE* v_out);
TYC_RESULT stack_pop(Stack* s, VALUE* v_out);

size_t     stack_len(Stack const* s);

TYC_RESULT stack_at(Stack const* s, int32_t key, VALUE* v);
TYC_RESULT stack_set(Stack* s, int32_t key, VALUE v);

size_t     stack_top_fp(Stack const* s);
TYC_RESULT stack_push_fp(Stack* s);
TYC_RESULT stack_pop_fp(Stack* s);
size_t     stack_fp_level(Stack const* s);

size_t     stack_collectable_array(Stack const* s, VALUE** values);

//
// HEAP ARRAY
//

typedef struct Array Array;

Array* array_new(void);
void   array_destroy(Array* a);

size_t array_len(Array const* a);
VALUE  array_get(Array const* a, size_t pos);
void   array_set(Array* a, size_t pos, VALUE v);
void   array_append(Array* a, VALUE v);

//
// HEAP TABLE
//

typedef struct Table Table;

Table* table_new(void);
void   table_destroy(Table* t);

size_t table_len(Table* t);
VALUE  table_get(Table const* t, VALUE key);
void   table_set(Table* t, VALUE key, VALUE value);

//
// HEAP
//

typedef struct Heap Heap;

typedef uint32_t HEAP_KEY;

Heap*    heap_new(void);
void     heap_destroy(Heap* h);

HEAP_KEY   heap_add_string(Heap* h, const char* value);
TYC_RESULT heap_get_string(Heap const* h, HEAP_KEY key, const char** value);

size_t   heap_size(Heap const* h);

void     heap_gc(Heap* h, VALUE const* roots, size_t n_roots);

#endif //TYCHE_PRIV_H
