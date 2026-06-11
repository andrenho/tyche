#include "../lib/priv.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//
// SUPPORT
//

#define EQ(a, b) (memcmp(a, b) == 0)


//
// INDIVIDUAL TESTS
//

static void test_values(void)
{
    printf("## Values\n");
    assert(value_type(create_value_integer(42)) == TYC_INTEGER);
    assert(value_integer(create_value_integer(-42)) == -42);
    assert(fabs(value_real(create_value_real(42.4)) - 42.4) < 0.00001);
    assert(value_function_idx(create_value_function_idx(42)) == 42);
    assert(value_heap_key(create_value_heap_key(TYC_STRING, 42)) == 42);
}

static void test_stack(void)
{
    {
        printf("## Stack\n");

        Stack* s = stack_new();

        stack_push(s, create_value_integer(10));
        stack_push(s, create_value_integer(20));
        stack_push(s, create_value_integer(30));

        VALUE v;
        assert(stack_size(s) == 3);
        assert(stack_at(s, 0, &v) == TYC_OK); assert(value_integer(v) == 10);
        assert(stack_at(s, 1, &v) == TYC_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -1, &v) == TYC_OK); assert(value_integer(v) == 30);
        assert(stack_at(s, -2, &v) == TYC_OK); assert(value_integer(v) == 20);

        abort_on_errors = false;
        assert(stack_at(s, 3, &v) == TYC_ERR);
        assert(stack_at(s, -4, &v) == TYC_ERR);

        assert(stack_set(s, 1, create_value_integer(99)) == TYC_OK);
        assert(stack_at(s, 1, &v) == TYC_OK); assert(value_integer(v) == 99);
        assert(stack_at(s, -2, &v) == TYC_OK); assert(value_integer(v) == 99);

        assert(stack_pop(s, NULL) == TYC_OK);
        assert(stack_pop(s, NULL) == TYC_OK);
        assert(stack_at(s, -1, &v) == TYC_OK); assert(value_integer(v) == 10);
        assert(stack_pop(s, NULL) == TYC_OK);
        assert(stack_size(s) == 0);

        assert(stack_pop(s, NULL) == TYC_ERR);

        abort_on_errors = true;

        stack_destroy(s);
    }

    {
        printf("## Stack with frame pointer\n");

        Stack* s = stack_new();

        stack_push(s, create_value_integer(10));
        stack_push(s, create_value_integer(20));
        stack_push_fp(s);
        stack_push(s, create_value_integer(30));
        stack_push(s, create_value_integer(40));
        stack_push(s, create_value_integer(50));

        VALUE v;
        assert(stack_size(s) == 3);
        assert(stack_at(s, 0, &v) == TYC_OK); assert(value_integer(v) == 30);
        assert(stack_at(s, 1, &v) == TYC_OK); assert(value_integer(v) == 40);
        assert(stack_at(s, -1, &v) == TYC_OK); assert(value_integer(v) == 50);
        assert(stack_at(s, -2, &v) == TYC_OK); assert(value_integer(v) == 40);

        assert(stack_set(s, -2, create_value_integer(99)) == TYC_OK);
        assert(stack_at(s, -2, &v) == TYC_OK); assert(value_integer(v) == 99);

        abort_on_errors = false;
        assert(stack_at(s, 3, &v) == TYC_ERR);
        assert(stack_at(s, -4, &v) == TYC_ERR);

        stack_pop_fp(s);

        assert(stack_size(s) == 2);
        assert(stack_at(s, 0, &v) == TYC_OK); assert(value_integer(v) == 10);
        assert(stack_at(s, 1, &v) == TYC_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -1, &v) == TYC_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -2, &v) == TYC_OK); assert(value_integer(v) == 10);

        assert(stack_at(s, 2, &v) == TYC_ERR);
        assert(stack_at(s, -3, &v) == TYC_ERR);
        abort_on_errors = true;

        stack_destroy(s);
    }
}

static void test_heap_strings(void)
{
    Heap* h = heap_new();

    HEAP_KEY v1 = heap_add_string(h, "Hello", true);
    HEAP_KEY v2 = heap_add_string(h, "Hello", false);
    assert(v1 == v2);
    assert(heap_size(h) == 1);

    HEAP_KEY v3 = heap_add_string(h, "World", false);
    assert(heap_size(h) == 2);

    const char *s1, *s2;
    heap_get_string(h, v1, &s1); assert(strcmp(s1, "Hello") == 0);
    heap_get_string(h, v3, &s2); assert(strcmp(s2, "World") == 0);

    heap_destroy(h);
}

static void test_arrays(void)
{
    printf("## Arrays\n");

    Array* a = array_new();
    assert(array_len(a) == 0);

    array_set(a, 1, create_value_integer(40));
    assert(array_len(a) == 2);
    assert(value_type(array_get(a, 0)) == TYC_NIL);
    assert(value_type(array_get(a, 1)) == TYC_INTEGER);

    array_append(a, create_value_integer(50));
    assert(array_len(a) == 3);
    assert(value_integer(array_get(a, 2)) == 50);

    array_set(a, 2, create_value_integer(60));
    assert(array_len(a) == 3);
    assert(value_integer(array_get(a, 2)) == 60);

    array_destroy(a);
}

static void test_tables(void)
{
    {
        printf("## Table - integer index\n");

        TycheVM* T = tyc_new();
        Table* t = table_new(T);

        table_set(t, create_value_integer(10), create_value_integer(100));
        table_set(t, create_value_integer(18), create_value_integer(200));

        VALUE v;
        assert(table_get(t, create_value_integer(10), &v)); assert(value_integer(v) == 100);
        assert(table_get(t, create_value_integer(18), &v)); assert(value_integer(v) == 200);

        table_del(t, create_value_integer(18));
        assert(table_get(t, create_value_integer(10), &v) && value_type(v) == TYC_INTEGER);
        assert(!table_get(t, create_value_integer(18), &v));

        table_destroy(t);
        tyc_destroy(T);
    }

    {
        printf("## Table - heavier usage\n");

        TycheVM* T = tyc_new();
        Table* t = table_new(T);
        for (size_t i = 0; i < 100; i += 2)
            table_set(t, create_value_integer(i), create_value_integer(i * 10));

        VALUE v;
        for (size_t i = 0; i < 100; i += 2) {
            assert(table_get(t, create_value_integer(i), &v));
            assert(value_type(v) == TYC_INTEGER);
            assert(value_integer(v) == i * 10);
        }

        table_destroy(t);
        tyc_destroy(T);
    }

    {
        printf("## Table - string index\n");

        TycheVM* T = tyc_new();
        Heap* h = tyc_heap(T);
        Table* t = table_new(T);

        VALUE key1 = create_value_heap_key(TYC_STRING, heap_add_string(h, "key1", false));
        VALUE key2 = create_value_heap_key(TYC_STRING, heap_add_string(h, "key2", false));

        table_set(t, key1, create_value_integer(100));
        table_set(t, key2, create_value_integer(200));

        VALUE key1b = create_value_heap_key(TYC_STRING, heap_add_string(h, "key1", false));
        VALUE key2b = create_value_heap_key(TYC_STRING, heap_add_string(h, "key2", false));

        VALUE v;
        assert(table_get(t, key1b, &v)); assert(value_integer(v) == 100);
        assert(table_get(t, key2b, &v)); assert(value_integer(v) == 200);

        table_del(t, key2b);
        assert(table_get(t, key1b, &v));
        table_get(t, key2b, &v); assert(value_type(v) == TYC_NIL);

        table_destroy(t);
        tyc_destroy(T);
    }

    {
        printf("## Table - next\n");

        TycheVM* T = tyc_new();
        Table* t = table_new(T);
        int count[10];

        for (size_t i = 0; i < 10; ++i) {
            table_set(t, create_value_integer(i), create_value_integer(i * 10));
            count[i] = 0;
        }

        VALUE k = create_value_nil(), v;
        while (table_next(t, k, &k, &v)) {
            assert(value_integer(k) == value_integer(v) / 10);
            ++count[value_integer(k)];
        }

        for (size_t i = 0; i < 10; ++i)
            assert(count[i] == 1);

        table_destroy(t);
        tyc_destroy(T);
    }

    {
        printf("## Table - next - with deletion\n");

        TycheVM* T = tyc_new();
        Table* t = table_new(T);
        int count[10];

        for (size_t i = 0; i < 10; ++i) {
            table_set(t, create_value_integer(i), create_value_integer(i * 10));
            count[i] = 0;
        }

        table_del(t, create_value_integer(6));

        VALUE k = create_value_nil(), v;
        while (table_next(t, k, &k, &v)) {
            assert(value_integer(k) == value_integer(v) / 10);
            ++count[value_integer(k)];
        }

        for (size_t i = 0; i < 10; ++i)
            if (i == 6)
                assert(count[i] == 0);
            else
                assert(count[i] == 1);

        table_destroy(t);
        tyc_destroy(T);
    }
}

static void test_heap(void)
{
    {
        printf("## Heap - string GC\n");

        Stack* s = stack_new();
        Heap* h = heap_new();

        stack_push(s, create_value_heap_key(TYC_STRING, heap_add_string(h, "item1", false)));
        stack_push(s, create_value_heap_key(TYC_STRING, heap_add_string(h, "item2", false)));
        stack_push(s, create_value_heap_key(TYC_STRING, heap_add_string(h, "item3", false)));

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

    {
        printf("## Heap - string dedup GC (1)\n");

        Stack* s = stack_new();
        Heap* h = heap_new();

        VALUE h1 = create_value_heap_key(TYC_STRING, heap_add_string(h, "item1", false));
        stack_push(s, h1);
        VALUE h2 = create_value_heap_key(TYC_STRING, heap_add_string(h, "item2", false));
        stack_push(s, h2);
        VALUE h3 = create_value_heap_key(TYC_STRING, heap_add_string(h, "item2", false));
        stack_push(s, h3);

        size_t v_sz;
        VALUE* v_idx;

        assert(heap_size(h) == 2);
        v_sz = stack_collectable_array(s, &v_idx);
        heap_gc(h, v_idx, v_sz);
        free(v_idx);
        assert(heap_size(h) == 2);

        stack_pop(s, NULL);

        assert(heap_size(h) == 2);
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

    {
        printf("## Heap - string dedup GC (2)\n");

        Stack* s = stack_new();
        Heap* h = heap_new();

        stack_push(s, create_value_heap_key(TYC_STRING, heap_add_string(h, "item1", false)));
        stack_push(s, create_value_heap_key(TYC_STRING, heap_add_string(h, "item2", false)));
        stack_push(s, create_value_heap_key(TYC_STRING, heap_add_string(h, "item2", true)));

        size_t v_sz;
        VALUE* v_idx;

        assert(heap_size(h) == 2);
        v_sz = stack_collectable_array(s, &v_idx);
        heap_gc(h, v_idx, v_sz);
        free(v_idx);
        assert(heap_size(h) == 2);

        stack_pop(s, NULL);

        assert(heap_size(h) == 2);
        v_sz = stack_collectable_array(s, &v_idx);
        heap_gc(h, v_idx, v_sz);
        free(v_idx);
        assert(heap_size(h) == 2);

        stack_pop(s, NULL);
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

    {
        printf("## Heap - array GC\n");

        Heap* h = heap_new();
        HEAP_KEY key = heap_add_array(h);
        VALUE array_val = create_value_heap_key(TYC_ARRAY, key);

        Array* array;
        assert(heap_get_array(h, key, &array) == TYC_OK);

        array_set(array, 0, create_value_integer(42));
        array_set(array, 1, create_value_heap_key(TYC_STRING, heap_add_string(h, "Hello", false)));
        array_set(array, 2, create_value_heap_key(TYC_STRING, heap_add_string(h, "World", false)));
        assert(heap_size(h) == 3);

        // set item 2, string "World" is now without reference
        array_set(array, 2, create_value_nil());
        assert(heap_size(h) == 3);
        heap_gc(h, &array_val, 1);
        assert(heap_size(h) == 2);

        // delete array, should delete the other string
        heap_gc(h, NULL, 0);
        assert(heap_size(h) == 0);

        heap_destroy(h);
    }

    {
        printf("## Heap - table GC\n");

        TycheVM* T = tyc_new();
        Heap* h = tyc_heap(T);
        HEAP_KEY key = heap_add_table(h, T);
        VALUE table_value = create_value_heap_key(TYC_TABLE, key);

        HEAP_KEY s1 = heap_add_string(h, "Hello", false);
        HEAP_KEY s2 = heap_add_string(h, "World", false);
        VALUE sv1 = create_value_heap_key(TYC_STRING, s1);
        VALUE sv2 = create_value_heap_key(TYC_STRING, s2);

        Table* table;
        assert(heap_get_table(h, key, &table) == TYC_OK);

        table_set(table, sv1, sv2);

        assert(heap_size(h) == 4);
        tyc_gc(T);
        assert(heap_size(h) == 1);

        tyc_destroy(T);
    }

    {
        printf("## Heap - table containing array containing string GC\n");

        TycheVM* T = tyc_new();
        Heap* h = tyc_heap(T);

        // table
        HEAP_KEY table_key = heap_add_table(h, T);
        VALUE table_value = create_value_heap_key(TYC_TABLE, table_key);
        Table* table;
        assert(heap_get_table(h, table_key, &table) == TYC_OK);

        // array
        HEAP_KEY array_key = heap_add_array(h);
        VALUE array_val = create_value_heap_key(TYC_ARRAY, array_key);
        Array* array;
        assert(heap_get_array(h, array_key, &array) == TYC_OK);

        // strings
        HEAP_KEY s1 = heap_add_string(h, "Hello", false);
        HEAP_KEY s2 = heap_add_string(h, "World", false);
        VALUE sv1 = create_value_heap_key(TYC_STRING, s1);
        VALUE sv2 = create_value_heap_key(TYC_STRING, s2);

        // table.Hello = ["World"]
        array_append(array, sv2);
        assert(table_len(table) == 0);
        table_set(table, sv1, array_val);
        assert(table_len(table) == 1);

        // initial situation: HEAP=4 + global
        assert(heap_size(h) == 5);
        stack_push(tyc_stack(T), table_value);
        tyc_gc(T);
        assert(heap_size(h) == 5);

        // remove table key, HEAP=1 + global now because only the empty table is left
        VALUE sv3 = create_value_heap_key(TYC_STRING, heap_add_string(h, "Hello", false));
        assert(value_heap_key(sv1) == value_heap_key(sv3));
        table_set(table, sv3, create_value_nil());
        assert(table_len(table) == 0);
        tyc_gc(T);
        assert(heap_size(h) == 2);

        // now the table is removed, and nothing is left
        stack_pop(tyc_stack(T), NULL);
        tyc_gc(T);
        assert(heap_size(h) == 1);

        heap_destroy(h);
    }
}

static void test_supertables(void)
{
    {
        printf("## Supertable\n");

        Heap* h = heap_new();

        // create table and supertable
        Table* super;
        HEAP_KEY super_heap_key = heap_add_table(h, NULL);
        heap_get_table(h, super_heap_key, &super);

        Table* table;
        HEAP_KEY table_heap_key = heap_add_table(h, NULL);
        heap_get_table(h, table_heap_key, &table);
        VALUE table_value = create_value_heap_key(TYC_TABLE, table_heap_key);

        // field names
        VALUE va = create_value_heap_key(TYC_STRING, heap_add_string(h, "va", false));
        VALUE vb = create_value_heap_key(TYC_STRING, heap_add_string(h, "vb", false));
        VALUE f1 = create_value_heap_key(TYC_STRING, heap_add_string(h, "f1", false));
        VALUE f99 = create_value_heap_key(TYC_STRING, heap_add_string(h, "f99", false));

        // add fields to supertable
        table_set(super, va, create_value_integer(20));
        table_set(super, vb, create_value_integer(30));
        table_set(super, f1, create_value_function_idx(1));
        table_set(super, f99, create_value_function_idx(99));

        // set table supertable
        heap_set_supertable(h, table_heap_key, super_heap_key);
        table_set(table, va, create_value_integer(40));

        // check table fields
        VALUE a;
        assert(table_get(table, va, &a) == TYC_OK); assert(value_integer(a) == 40);
        assert(table_get(table, vb, &a) == TYC_OK); assert(value_integer(a) == 30);
        assert(table_get(table, f1, &a) == TYC_OK); assert(value_function_idx(a) == 1);

        // overload function in table
        table_set(table, f1, create_value_function_idx(2));
        assert(table_get(table, f1, &a) == TYC_OK); assert(value_function_idx(a) == 2);
        assert(table_get(super, f1, &a) == TYC_OK); assert(value_function_idx(a) == 1);

        // test iteration
        bool found_va = false, found_vb = false, found_f1 = false, found_f99 = false;
        VALUE key = create_value_nil(), value;
        while (table_next(table, key, &key, &value)) {
            // const char* str; heap_get_string(h, value_heap_key(key), &str); printf("- %s\n", str);
            if (value_heap_key(key) == value_heap_key(va)) {
                found_va = true;
                assert(value_integer(value) == 40);
            }
            if (value_heap_key(key) == value_heap_key(vb)) {
                found_vb = true;
                assert(value_integer(value) == 30);
            }
            if (value_heap_key(key) == value_heap_key(f1)) {
                found_f1 = true;
                assert(value_function_idx(value) == 2);
            }
            if (value_heap_key(key) == value_heap_key(f99)) {
                found_f99 = true;
                assert(value_function_idx(value) == 99);
            }
        }
        assert(found_va);
        assert(found_vb);
        assert(found_f1);
        assert(found_f99);

        // restore overloaded function
        table_set(table, f1, create_value_nil());
        assert(table_get(table, f1, &a) == TYC_OK); assert(value_function_idx(a) == 1);

        // test gc
        assert(heap_size(h) == 6);
        heap_gc(h, &table_value, 1);
        assert(heap_size(h) == 6);
        heap_gc(h, NULL, 0);
        assert(heap_size(h) == 0);

        heap_destroy(h);
    }

    // TODO - test supertable iteration
}

static void test_vm(void)
{
    {
        printf("## VM - Basic\n");

        TycheVM* T = tyc_new();

        tyc_pushinteger(T, 2);
        tyc_pushinteger(T, 3);
        assert(tyc_expr(T, TX_SUM) == TYC_OK);
        int32_t result; assert(tyc_tointeger(T, -1, &result) == TYC_OK);
        assert(result == 5);

        tyc_destroy(T);
    }

    {
        printf("## Managed strings\n");
        TycheVM* T = tyc_new();
        tyc_pushstring(T, "Hello");
        const char* result; tyc_tostring(T, -1, &result);
        assert(strcmp(result, "Hello") == 0);
        tyc_destroy(T);
    }
}

static TYC_RESULT test_function(TycheVM* T)
{
    int x;
    assert(tyc_tointeger(T, 0, &x) == TYC_OK);
    tyc_pushinteger(T, x + 5);
    return TYC_OK;
}

static void test_native_pointer(void)
{
    {
        printf("## Native pointer - variable\n");
        int i = 45;

        TycheVM* T = tyc_new();
        tyc_pushnativeptr(T, &i);

        void* ptr;
        assert(tyc_tonativeptr(T, -1, &ptr) == TYC_OK);
        assert((*(int *) ptr) == 45);
        tyc_pop(T);

        tyc_destroy(T);
    }

    {
        printf("## Native pointer - function\n");

        TycheVM* T = tyc_new();
        tyc_pushnativefunction(T, test_function);

        tyc_pushinteger(T, 12);
        assert(tyc_call(T, 1) == TYC_OK);

        int x; assert(tyc_tointeger(T, -1, &x) == TYC_OK);
        assert(x == 17);

        tyc_destroy(T);
    }
}

static void test_hashes()
{
    printf("## Hashes\n");

    TycheVM* T = tyc_new();
    printf("  - Nil: 0x%x\n", tyc_hash(T, create_value_nil()));
    printf("  - True: 0x%x\n", tyc_hash(T, create_value_bool(true)));
    printf("  - False: 0x%x\n", tyc_hash(T, create_value_bool(false)));
    printf("  - Integer: 0x%x\n", tyc_hash(T, create_value_integer(567)));
    printf("  - Real: 0x%x\n", tyc_hash(T, create_value_real(3.14)));

    HEAP_KEY k = heap_add_string(tyc_heap(T), "Hello world", false);
    uint32_t h1 = tyc_hash(T, create_value_heap_key(TYC_STRING, k));
    HEAP_KEY k2 = heap_add_string(tyc_heap(T), "Hello world", false);
    uint32_t h2 = tyc_hash(T, create_value_heap_key(TYC_STRING, k2));
    printf("  - String: 0x%x, 0x%x\n", h1, h2);
    assert(h1 == h2);

    k = heap_add_array(tyc_heap(T));
    printf("  - Array: 0x%x, 0x%x\n",
        tyc_hash(T, create_value_heap_key(TYC_ARRAY, k)),
        tyc_hash(T, create_value_heap_key(TYC_ARRAY, k))
    );

    k = heap_add_array(tyc_heap(T));
    printf("  - 2nd array: 0x%x, 0x%x\n", tyc_hash(T, create_value_heap_key(TYC_ARRAY, k)));

    k = heap_add_table(tyc_heap(T), T);
    printf("  - Table: 0x%x\n", tyc_hash(T, create_value_heap_key(TYC_TABLE, k)));

    tyc_destroy(T);
}

//
// MAIN
//

int main(void)
{
    test_values();
    test_stack();
    test_heap_strings();
    test_arrays();
    test_tables();
    test_heap();
    test_supertables();
    test_vm();
    test_native_pointer();
    test_hashes();
}
