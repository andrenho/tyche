#include "../lib/priv.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

//
// SUPPORT
//

#define EQ(a, b) (memcmp(a, b) == 0)

static void check_expected_top(lua_State* L, TycheVM* T)
{
    // check stack size
    lua_getfield(L, -1, "expected_stack_size");
    if (!lua_isnil(L, -1))
        assert(tyc_stack_size(T) == (size_t) lua_tointeger(L, -1));
    lua_pop(L, 1);

    // check stack top
    lua_getfield(L, -1, "expected_stack_top");
    if (lua_isinteger(L, -1)) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == T_OK); assert(type == TT_INTEGER);
        int32_t v; assert(tyc_tointeger(T, -1, &v) == T_OK); assert(v == lua_tointeger(L, -1));
    } else if (lua_isstring(L, -1)) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == T_OK); assert(type == TT_STRING);
        const char* str; assert(tyc_tostring(T, -1, &str) == T_OK); assert(strcmp(str, lua_tostring(L, -1)) == 0);
    } else if (lua_isboolean(L, -1)) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == T_OK); assert(type == TT_BOOLEAN);
        bool v; assert(tyc_toboolean(T, -1, &v) == T_OK); assert(v == lua_toboolean(L, -1));
    } else if (!lua_isnil(L, -1)) {
        abort();
    }
    lua_pop(L, 1);

    // check heap size
    lua_getfield(L, -1, "expected_heap_size");
    if (!lua_isnil(L, -1))
        assert(heap_size(tyc_heap(T)) == (size_t) lua_tointeger(L, -1));
    lua_pop(L, 1);
}

static void run_assembly_test_code(lua_State* L, bool debug, bool decompile, bool debug_bytecode)
{
    TycheVM* T = tyc_new();
    tyc_debug_to_console(T, debug);

    // load code
    uint8_t* bytecode; size_t bytecode_sz;
    lua_getfield(L, -1, "code");
    assert(code_assemble(lua_tostring(L, -1), &bytecode, &bytecode_sz) == T_OK);
    lua_pop(L, 1);

    // run code
    assert(tyc_load_bytecode(T, bytecode, bytecode_sz) == T_OK);
    if (debug_bytecode)
        tyc_print_bytecode(T);
    if (decompile)
        tyc_assembly_decompile(T);
    assert(tyc_call(T, 0) == T_OK);

    // assert
    check_expected_top(L, T);

    // cleanup
    free(bytecode);
    tyc_destroy(T);
}

static void run_assembly_test_template(lua_State* L, bool debug, bool decompile, bool debug_bytecode)
{
    lua_getfield(L, -1, "template");
    char* template = strdup(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "scenarios");
    assert(!lua_isnil(L, -1));

    long n_scenarios = luaL_len(L, -1);
    for (long i = 0; i < n_scenarios; ++i) {
        lua_geti(L, -1, (int)i + 1);

        lua_getfield(L, -1, "name");
        printf("     .. %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);

        // format code
        luaL_dostring(L, "return string.format");
        assert(lua_isfunction(L, -1));
        lua_pushstring(L, template);

        lua_getfield(L, -3, "parameters");
        assert(!lua_isnil(L, -1));
        int n_params = (int) luaL_len(L, -1);
        for (int j = 0; j < n_params; ++j)
            lua_geti(L, -(j + 1), j + 1);
        lua_remove(L, -(n_params + 1));

        lua_call(L, n_params + 1, 1);
        char* formatted_code = strdup(lua_tostring(L, -1));
        lua_pop(L, 1);

        // run code
        TycheVM* T = tyc_new();
        tyc_debug_to_console(T, debug);
        uint8_t* bytecode; size_t bytecode_sz;
        assert(code_assemble(formatted_code, &bytecode, &bytecode_sz) == T_OK);
        assert(tyc_load_bytecode(T, bytecode, bytecode_sz) == T_OK);
        if (debug_bytecode)
            tyc_print_bytecode(T);
        if (decompile)
            tyc_assembly_decompile(T);
        assert(tyc_call(T, 0) == T_OK);

        // assert
        check_expected_top(L, T);

        // cleanup
        free(bytecode);
        tyc_destroy(T);
        free(formatted_code);
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    free(template);
}

static void run_assembly_test(lua_State* L)
{
    // print test name
    lua_getfield(L, -1, "name");
    printf("   - %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);

    // debug?
    lua_getfield(L, -1, "debug");
    bool debug = lua_isboolean(L, -1) && lua_toboolean(L, -1);
    lua_pop(L, 1);

    // decompile?
    lua_getfield(L, -1, "decompile");
    bool decompile = lua_isboolean(L, -1) && lua_toboolean(L, -1);
    lua_pop(L, 1);

    // decompile?
    lua_getfield(L, -1, "debug_bytecode");
    bool debug_bytecode = lua_isboolean(L, -1) && lua_toboolean(L, -1);
    lua_pop(L, 1);

    // has code?
    lua_getfield(L, -1, "code");
    if (!lua_isnil(L, -1)) {
        lua_pop(L, 1);
        run_assembly_test_code(L, debug, decompile, debug_bytecode);
        return;
    } else {
        lua_pop(L, 1);
    }

    // has template
    lua_getfield(L, -1, "template");
    if (!lua_isnil(L, -1)) {
        lua_pop(L, 1);
        run_assembly_test_template(L, debug, decompile, debug_bytecode);
    } else {
        lua_pop(L, 1);
    }
}

static void run_assembly_tests(void)
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    int r = luaL_loadfile(L, "./test/code-tests.lua");
    assert(r == LUA_OK);
    lua_call(L, 0, 1);
    assert(lua_istable(L, -1));

    size_t len = (size_t) luaL_len(L, -1);
    for (size_t i = 0; i < len; ++i) {
        lua_geti(L, -1, (int)i + 1);
        run_assembly_test(L);
        lua_pop(L, 1);
    }

    lua_close(L);
}


//
// INDIVIDUAL TESTS
//

static void test_values(void)
{
    printf("## Values\n");
    assert(value_type(create_value_integer(42)) == TT_INTEGER);
    assert(value_integer(create_value_integer(-42)) == -42);
    assert(fabs(value_real(create_value_real(42.4)) - 42.4) < 0.00001);
    assert(value_function_idx(create_value_function_idx(42)) == 42);
    assert(value_heap_key(create_value_heap_key(TT_STRING, 42)) == 42);
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
        assert(stack_at(s, 0, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_at(s, 1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -1, &v) == T_OK); assert(value_integer(v) == 30);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 20);

        abort_on_errors = false;
        assert(stack_at(s, 3, &v) == T_ERR);
        assert(stack_at(s, -4, &v) == T_ERR);

        assert(stack_set(s, 1, create_value_integer(99)) == T_OK);
        assert(stack_at(s, 1, &v) == T_OK); assert(value_integer(v) == 99);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 99);

        assert(stack_pop(s, NULL) == T_OK);
        assert(stack_pop(s, NULL) == T_OK);
        assert(stack_at(s, -1, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_pop(s, NULL) == T_OK);
        assert(stack_size(s) == 0);

        assert(stack_pop(s, NULL) == T_ERR);

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
        assert(stack_at(s, 0, &v) == T_OK); assert(value_integer(v) == 30);
        assert(stack_at(s, 1, &v) == T_OK); assert(value_integer(v) == 40);
        assert(stack_at(s, -1, &v) == T_OK); assert(value_integer(v) == 50);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 40);

        assert(stack_set(s, -2, create_value_integer(99)) == T_OK);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 99);

        abort_on_errors = false;
        assert(stack_at(s, 3, &v) == T_ERR);
        assert(stack_at(s, -4, &v) == T_ERR);

        stack_pop_fp(s);

        assert(stack_size(s) == 2);
        assert(stack_at(s, 0, &v) == T_OK); assert(value_integer(v) == 10);
        assert(stack_at(s, 1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -1, &v) == T_OK); assert(value_integer(v) == 20);
        assert(stack_at(s, -2, &v) == T_OK); assert(value_integer(v) == 10);

        assert(stack_at(s, 2, &v) == T_ERR);
        assert(stack_at(s, -3, &v) == T_ERR);
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

static void test_tables(void)
{
    {
        printf("## Table - integer index\n");

        Heap* h = heap_new();
        Table* t = table_new(h);

        table_set(t, create_value_integer(10), create_value_integer(100));
        table_set(t, create_value_integer(20), create_value_integer(200));

        VALUE v;
        assert(table_get(t, create_value_integer(10), &v) == T_OK); assert(value_integer(v) == 100);
        assert(table_get(t, create_value_integer(20), &v) == T_OK); assert(value_integer(v) == 200);

        table_del(t, create_value_integer(20));
        assert(table_get(t, create_value_integer(10), &v) == T_OK);
        assert(table_get(t, create_value_integer(20), &v) == T_OK && value_type(v) == TT_NIL);

        table_destroy(t);
        heap_destroy(h);
    }

    {
        printf("## Table - string index\n");

        Heap* h = heap_new();
        Table* t = table_new(h);

        VALUE key1 = create_value_heap_key(TT_STRING, heap_add_string(h, "key1", false));
        VALUE key2 = create_value_heap_key(TT_STRING, heap_add_string(h, "key2", false));

        table_set(t, key1, create_value_integer(100));
        table_set(t, key2, create_value_integer(200));

        VALUE key1b = create_value_heap_key(TT_STRING, heap_add_string(h, "key1", false));
        VALUE key2b = create_value_heap_key(TT_STRING, heap_add_string(h, "key2", false));

        VALUE v;
        assert(table_get(t, key1b, &v) == T_OK); assert(value_integer(v) == 100);
        assert(table_get(t, key2b, &v) == T_OK); assert(value_integer(v) == 200);

        table_del(t, key2b);
        assert(table_get(t, key1b, &v) == T_OK);
        table_get(t, key2b, &v); assert(value_type(v) == TT_NIL);

        table_destroy(t);
        heap_destroy(h);
    }
}

static void test_heap(void)
{
    {
        printf("## Heap - string GC\n");

        Stack* s = stack_new();
        Heap* h = heap_new();

        stack_push(s, create_value_heap_key(TT_STRING, heap_add_string(h, "item1", false)));
        stack_push(s, create_value_heap_key(TT_STRING, heap_add_string(h, "item2", false)));
        stack_push(s, create_value_heap_key(TT_STRING, heap_add_string(h, "item3", false)));

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
        printf("## Heap - string dedup GC\n");

        Stack* s = stack_new();
        Heap* h = heap_new();

        VALUE h1 = create_value_heap_key(TT_STRING, heap_add_string(h, "item1", false));
        stack_push(s, h1);
        VALUE h2 = create_value_heap_key(TT_STRING, heap_add_string(h, "item2", false));
        stack_push(s, h2);
        VALUE h3 = create_value_heap_key(TT_STRING, heap_add_string(h, "item2", false));
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
        printf("## Heap - string dedup GC\n");

        Stack* s = stack_new();
        Heap* h = heap_new();

        stack_push(s, create_value_heap_key(TT_STRING, heap_add_string(h, "item1", false)));
        stack_push(s, create_value_heap_key(TT_STRING, heap_add_string(h, "item2", false)));
        stack_push(s, create_value_heap_key(TT_STRING, heap_add_string(h, "item2", true)));

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
        VALUE array_val = create_value_heap_key(TT_ARRAY, key);

        Array* array;
        assert(heap_get_array(h, key, &array) == T_OK);

        array_set(array, 0, create_value_integer(42));
        array_set(array, 1, create_value_heap_key(TT_STRING, heap_add_string(h, "Hello", false)));
        array_set(array, 2, create_value_heap_key(TT_STRING, heap_add_string(h, "World", false)));
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
        printf("## Heap - array GC\n");

        Heap* h = heap_new();
        HEAP_KEY key = heap_add_table(h);
        VALUE table_value = create_value_heap_key(TT_TABLE, key);

        HEAP_KEY s1 = heap_add_string(h, "Hello", false);
        HEAP_KEY s2 = heap_add_string(h, "World", false);
        VALUE sv1 = create_value_heap_key(TT_STRING, s1);
        VALUE sv2 = create_value_heap_key(TT_STRING, s2);

        Table* table;
        assert(heap_get_table(h, key, &table) == T_OK);

        table_set(table, sv1, sv2);

        assert(heap_size(h) == 3);
        heap_gc(h, &table_value, 1);
        assert(heap_size(h) == 3);

        heap_destroy(h);
    }

    {
        printf("## Heap - table containing array containing string GC\n");

        Heap* h = heap_new();

        // table
        HEAP_KEY table_key = heap_add_table(h);
        VALUE table_value = create_value_heap_key(TT_TABLE, table_key);
        Table* table;
        assert(heap_get_table(h, table_key, &table) == T_OK);

        // array
        HEAP_KEY array_key = heap_add_array(h);
        VALUE array_val = create_value_heap_key(TT_ARRAY, array_key);
        Array* array;
        assert(heap_get_array(h, array_key, &array) == T_OK);

        // strings
        HEAP_KEY s1 = heap_add_string(h, "Hello", false);
        HEAP_KEY s2 = heap_add_string(h, "World", false);
        VALUE sv1 = create_value_heap_key(TT_STRING, s1);
        VALUE sv2 = create_value_heap_key(TT_STRING, s2);

        // table.Hello = ["World"]
        array_append(array, sv2);
        assert(table_size(table) == 0);
        table_set(table, sv1, array_val);
        assert(table_size(table) == 1);

        // initial situation: HEAP=4
        assert(heap_size(h) == 4);
        heap_gc(h, &table_value, 1);
        assert(heap_size(h) == 4);

        // remove table key, HEAP=1 now because only the empty table is left
        VALUE sv3 = create_value_heap_key(TT_STRING, heap_add_string(h, "Hello", false));
        assert(value_heap_key(sv1) == value_heap_key(sv3));
        table_set(table, sv3, create_value_nil());
        assert(table_size(table) == 0);
        heap_gc(h, &table_value, 1);
        assert(heap_size(h) == 1);

        // now the table is removed, and nothing is left
        heap_gc(h, NULL, 0);
        assert(heap_size(h) == 0);

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
        HEAP_KEY super_heap_key = heap_add_table(h);
        heap_get_table(h, super_heap_key, &super);

        Table* table;
        HEAP_KEY table_heap_key = heap_add_table(h);
        heap_get_table(h, table_heap_key, &table);
        VALUE table_value = create_value_heap_key(TT_TABLE, table_heap_key);

        // field names
        VALUE va = create_value_heap_key(TT_STRING, heap_add_string(h, "va", false));
        VALUE vb = create_value_heap_key(TT_STRING, heap_add_string(h, "vb", false));
        VALUE f1 = create_value_heap_key(TT_STRING, heap_add_string(h, "f1", false));
        VALUE f99 = create_value_heap_key(TT_STRING, heap_add_string(h, "f99", false));

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
        assert(table_get(table, va, &a) == T_OK); assert(value_integer(a) == 40);
        assert(table_get(table, vb, &a) == T_OK); assert(value_integer(a) == 30);
        assert(table_get(table, f1, &a) == T_OK); assert(value_function_idx(a) == 1);

        // overload function in table
        table_set(table, f1, create_value_function_idx(2));
        assert(table_get(table, f1, &a) == T_OK); assert(value_function_idx(a) == 2);
        assert(table_get(super, f1, &a) == T_OK); assert(value_function_idx(a) == 1);

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
        assert(table_get(table, f1, &a) == T_OK); assert(value_function_idx(a) == 1);

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

static void test_bytecode(void)
{
    {
        printf("## Bytecode\n");
        const char* assembly_code =
                ".const\n"
                "    0: 3.14\n"
                "    1: \"Hello world\"\n"
                "\n"
                ".func 0\n"
                "    pushi   2   ; this is a comment\n"
                "    pushi   -3\n"
                "    sum\n"
                "    ret\n"
                ".func 1\n"
                "    pushi   5000\n"
                "    ret";

        uint8_t* bytecode; size_t bytecode_sz;
        assert(code_assemble(assembly_code, &bytecode, &bytecode_sz) == T_OK);

        Code* code = code_new();

        assert(code_load_bytecode(code, bytecode, bytecode_sz) == T_OK);

        assert(code_n_consts(code) == 2);
        assert(code_const_type(code, 0) == TC_REAL);
        assert(code_const_type(code, 1) == TC_STRING);
        assert(code_const_real(code, 0) > 3.13 && code_const_real(code, 0) < 3.15);
        assert(strcmp(code_const_string(code, 1), "Hello world") == 0);
        assert(code_n_functions(code) == 2);
        assert(code_function_sz(code, 0) == 6);
        assert(code_function_sz(code, 1) == 4);

        uint32_t addr = 0;
        Instruction inst = code_next_instruction(code, 0, addr);
        assert(inst.operator == TO_PUSHI);
        assert(inst.operand == 2);
        assert(inst.sz == 2);
        addr += inst.sz;

        inst = code_next_instruction(code, 0, addr);
        assert(inst.operator == TO_PUSHI);
        assert(inst.operand == -3);
        addr += inst.sz;

        inst = code_next_instruction(code, 0, addr);
        assert(inst.operator == TO_SUM);
        assert(inst.operand == 0);
        addr += inst.sz;

        inst = code_next_instruction(code, 1, 0);
        assert(inst.operator == TO_PUSHI);
        assert(inst.operand == 5000);
        assert(inst.sz == 3);

        code_destroy(code);
        free(bytecode);
    }

    {
        printf("## Bytecode - labels\n");
        const char* assembly_code =
                ".func 0\n"
                "    jmp    @my_label\n"
                "    pushi  \n"
                "@my_label:\n"
                "    ret";

        uint8_t* bytecode; size_t bytecode_sz;
        assert(code_assemble(assembly_code, &bytecode, &bytecode_sz) == T_OK);

        Code* code = code_new();
        assert(code_load_bytecode(code, bytecode, bytecode_sz) == T_OK);

        Instruction inst = code_next_instruction(code, 0, 0);
        assert(inst.operator == TO_JMP);
        assert(inst.operand == 4);
        assert(inst.sz == 3);

        code_destroy(code);
        free(bytecode);
    }
}

static void test_vm(void)
{
    {
        printf("## VM - Basic\n");

        TycheVM* T = tyc_new();

        tyc_pushinteger(T, 2);
        tyc_pushinteger(T, 3);
        assert(tyc_expr(T, TX_SUM) == T_OK);
        int32_t result; assert(tyc_tointeger(T, -1, &result) == T_OK);
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

    {
        printf("## Assembly tests\n");
        run_assembly_tests();
    }
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
    test_bytecode();
    test_vm();
}
