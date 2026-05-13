#include "../lib/priv.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EQ(a, b) (memcmp(a, b) == 0)

int main()
{
    {
        printf("### Values\n");
        assert(value_type(create_value_integer(42)) == TT_INTEGER);
        assert(value_integer(create_value_integer(-42)) == -42);
        assert(fabsf(value_real(create_value_real(42.4f)) - 42.4f) < 0.00001f);
        assert(value_idx(create_value_idx(TT_FUNCTION, 42)) == 42);
    }

    {
        printf("### Stack\n");

        Stack* s = stack_new();

        stack_push(s, create_value_integer(10));
        stack_push(s, create_value_integer(20));
        stack_push(s, create_value_integer(30));

        VALUE v;
        assert(stack_len(s) == 3);
        assert(stack_at(s, 0, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_at(s, 1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -1, &v) == T_OK); assert(value_integer(v) == 30);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 20);

        assert(stack_at(s, 3, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);
        assert(stack_at(s, -4, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);

        assert(stack_set(s, 1, create_value_integer(99)) == T_OK);
        assert(stack_at(s, 1, &v) == T_OK); assert(value_integer(v) == 99);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 99);

        assert(stack_pop(s, NULL) == T_OK);
        assert(stack_pop(s, NULL) == T_OK);
        assert(stack_at(s, -1, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_pop(s, NULL) == T_OK);
        assert(stack_len(s) == 0);

        assert(stack_pop(s, NULL) == T_ERR_STACK_UNDERFLOW);

        stack_destroy(s);
    }

    {
        printf("### Stack with frame pointer\n");

        Stack* s = stack_new();

        stack_push(s, create_value_integer(10));
        stack_push(s, create_value_integer(20));
        stack_push_fp(s);
        stack_push(s, create_value_integer(30));
        stack_push(s, create_value_integer(40));
        stack_push(s, create_value_integer(50));

        VALUE v;
        assert(stack_len(s) == 3);
        assert(stack_at(s, 0, &v) == T_OK); assert(value_integer(v) == 30);
        assert(stack_at(s, 1, &v) == T_OK); assert(value_integer(v) == 40);
        assert(stack_at(s, -1, &v) == T_OK); assert(value_integer(v) == 50);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 40);

        assert(stack_set(s, -2, create_value_integer(99)) == T_OK);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 99);

        assert(stack_at(s, 3, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);
        assert(stack_at(s, -4, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);

        stack_pop_fp(s);

        assert(stack_len(s) == 2);
        assert(stack_at(s, 0, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_at(s, 1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 10);

        assert(stack_at(s, 2, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);
        assert(stack_at(s, -3, &v) == T_ERR_STACK_ACCESS_OUT_OF_RANGE);

        stack_destroy(s);
    }

    {
        printf("### Arrays\n");

        Array* a = array_new();
        assert(array_len(a) == 0);

        array_set(a, 1, create_value_integer(40));
        assert(array_len(a) == 2);
        assert(value_type(array_get(a, 0)) == TT_NIL);
        assert(value_type(array_get(a, 1)) == TT_INTEGER);

        array_append(a, create_value_integer(50));
        assert(array_len(a) == 3);
        assert(value_integer(array_get(a, 2)) == 50);

        array_set(a, 2, create_value_integer(60));
        assert(array_len(a) == 3);
        assert(value_integer(array_get(a, 2)) == 60);

        array_destroy(a);
    }

    {
        printf("### Table - integer index\n");

        Heap* h = heap_new();
        Table* t = table_new(h);

        table_set(t, create_value_integer(10), create_value_integer(100));
        table_set(t, create_value_integer(20), create_value_integer(200));

        VALUE v;
        assert(table_get(t, create_value_integer(10), &v) == T_OK); assert(value_integer(v) == 100);
        assert(table_get(t, create_value_integer(20), &v) == T_OK); assert(value_integer(v) == 200);

        table_del(t, create_value_integer(20));
        assert(table_get(t, create_value_integer(10), &v) == T_OK);
        assert(table_get(t, create_value_integer(20), &v) == T_ERR_TABLE_KEY_NOT_FOUND);

        table_destroy(t);
        heap_destroy(h);
    }

    {
        printf("### Heap - strings\n");

        Heap* h = heap_new();

        HEAP_KEY key1 = heap_add_string(h, "hello");
        HEAP_KEY key2 = heap_add_string(h, "world");

        const char* value;
        assert(heap_get_string(h, key1, &value) == T_OK); assert(strcmp(value, "hello") == 0);
        assert(heap_get_string(h, key2, &value) == T_OK); assert(strcmp(value, "world") == 0);
        assert(heap_get_string(h, 1000, &value) == T_ERR_HEAP_KEY_NOT_FOUND);

        heap_destroy(h);
    }

    {
        printf("### Heap - string GC\n");

        Stack* s = stack_new();
        Heap* h = heap_new();

        stack_push(s, create_value_idx(TT_STRING, heap_add_string(h, "item1")));
        stack_push(s, create_value_idx(TT_STRING, heap_add_string(h, "item2")));
        stack_push(s, create_value_idx(TT_STRING, heap_add_string(h, "item3")));

        size_t v_sz;
        VALUE* v_idx;

        assert(heap_size(h) == 3);
        v_sz = stack_collectable_array(s, &v_idx);
        heap_gc(h, v_idx, v_sz);
        free(v_idx);
        assert(heap_size(h) == 3);

        stack_pop(s, NULL);

        assert(heap_size(h) == 3);
        v_sz = stack_collectable_array(s, &v_idx);
        heap_gc(h, v_idx, v_sz);
        free(v_idx);
        assert(heap_size(h) == 2);

        stack_pop(s, NULL);
        v_sz = stack_collectable_array(s, &v_idx);
        heap_gc(h, v_idx, v_sz);
        free(v_idx);
        assert(heap_size(h) == 1);

        heap_destroy(h);
        stack_destroy(s);
    }
}