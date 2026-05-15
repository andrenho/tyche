#include "../lib/priv.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define EQ(a, b) (memcmp(a, b) == 0)

static void run_assembly_tests(void);
static void run_assembly_test(lua_State* L);

int main()
{
    {
        printf("## Values\n");
        assert(value_type(create_value_integer(42)) == TT_INTEGER);
        assert(value_integer(create_value_integer(-42)) == -42);
        assert(fabsf(value_real(create_value_real(42.4f)) - 42.4f) < 0.00001f);
        assert(value_idx(create_value_idx(TT_FUNCTION, 42)) == 42);
    }

    {
        printf("## Stack\n");

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
        printf("## Stack with frame pointer\n");

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
        assert(table_get(t, create_value_integer(20), &v) == T_ERR_TABLE_KEY_NOT_FOUND);

        table_destroy(t);
        heap_destroy(h);
    }

    {
        printf("## Table - string index\n");

        Heap* h = heap_new();
        Table* t = table_new(h);

        VALUE key1 = create_value_idx(TT_STRING, heap_add_string(h, "key1"));
        VALUE key2 = create_value_idx(TT_STRING, heap_add_string(h, "key2"));

        table_set(t, key1, create_value_integer(100));
        table_set(t, key2, create_value_integer(200));

        VALUE key1b = create_value_idx(TT_STRING, heap_add_string(h, "key1"));
        VALUE key2b = create_value_idx(TT_STRING, heap_add_string(h, "key2"));

        VALUE v;
        assert(table_get(t, key1b, &v) == T_OK); assert(value_integer(v) == 100);
        assert(table_get(t, key2b, &v) == T_OK); assert(value_integer(v) == 200);

        table_del(t, key2b);
        assert(table_get(t, key1b, &v) == T_OK);
        assert(table_get(t, key2b, &v) == T_ERR_TABLE_KEY_NOT_FOUND);

        table_destroy(t);
        heap_destroy(h);
    }

    {
        printf("## Heap - strings\n");

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
        printf("## Heap - string GC\n");

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
        assert(code_const_real(code, 0) > 3.13f && code_const_real(code, 0) < 3.15f);
        assert(strcmp(code_const_string(code, 1), "Hello world") == 0);
        assert(code_n_functions(code) == 2);

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
        assert(inst.operand == 3);
        assert(inst.sz == 3);

        code_destroy(code);
        free(bytecode);
    }

    {
        printf("## VM - Basic\n");

        TycheVM* T = tyc_new();

        tyc_pushinteger(T, 2);
        tyc_pushinteger(T, 3);
        tyc_expr(T, TX_SUM);
        int32_t result; assert(tyc_tointeger(T, -1, &result) == T_OK);
        assert(result == 5);

        tyc_destroy(T);
    }

    {
        printf("## Assembly tests\n");
        run_assembly_tests();
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

    int len = luaL_len(L, -1);
    for (int i = 0; i < len; ++i) {
        lua_geti(L, -1, i + 1);
        run_assembly_test(L);
        lua_pop(L, 1);
    }

    lua_close(L);
}

static void run_assembly_test(lua_State* L)
{
    lua_getfield(L, -1, "name");
    printf("   - %s\n", lua_tostring(L, -1));
    lua_pop(L, -1);
}