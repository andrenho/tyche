#ifndef TYCHE_PRIV_H
#define TYCHE_PRIV_H

#include "tyche.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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
int32_t  value_integer(VALUE v);
float    value_real(VALUE v);
uint32_t value_idx(VALUE v);
bool     value_is_zero(VALUE v);

VALUE create_value_nil();
VALUE create_value_integer(int32_t v);
VALUE create_value_real(float f);
VALUE create_value_idx(TYC_TYPE type, uint32_t idx);

//
// STACK
//

typedef struct Stack Stack;

Stack*     stack_new();
void       stack_destroy(Stack* s);

TYC_RESULT stack_push(Stack* s, VALUE v);
TYC_RESULT stack_peek(Stack* s, VALUE* v_out);
TYC_RESULT stack_pop(Stack* s, VALUE* v_out);

size_t     stack_len(Stack* s);

TYC_RESULT stack_at(Stack* s, int32_t key, VALUE* v);
TYC_RESULT stack_set(Stack* s, int32_t key, VALUE v);

size_t     stack_top_fp(Stack* s);
TYC_RESULT stack_push_fp(Stack* s);
TYC_RESULT stack_pop_fp(Stack* s);
size_t     stack_fp_level(Stack* s);

//
// HEAP
//

typedef struct Heap Heap;

typedef int HEAP_KEY;

Heap*    heap_new();
void     heap_destroy(Heap* h);

HEAP_KEY   heap_add_string(Heap* h, const char* value);
TYC_RESULT heap_get_string(Heap* h, HEAP_KEY key, const char** value);

size_t   heap_size(Heap* h);

void     heap_gc(Heap* h, VALUE const* roots, size_t n_roots);

#endif //TYCHE_PRIV_H
