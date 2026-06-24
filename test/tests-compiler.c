#include "../lib/compiler/compiler_priv.h"
#include "../lib/priv.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include "toml-c.h"

//
// INDIVIDUAL TESTS
//

static void test_lexer(void)
{
    Token t;

    {
        printf("## Lexer\n");

        int col, line;

        const char* code =
            "func x() {\n"
            "  a == \"Hel\\\"lo\" + 13.8 * -0x12;  // test\n"
            "}";

        Lexer* L = lexer_init(code);

        assert(lexer_peek(L).type == TK_NOT_STARTED);
        lexer_pos(L, &col, &line); assert(col == 1 && line == 1);

        t = lexer_next(L); assert(t.type == TK_IDENTIFIER); assert(token_is(L, t, "func"));
        lexer_pos(L, &col, &line); assert(col == 1 && line == 1);

        t = lexer_next(L); assert(t.type == TK_IDENTIFIER); assert(token_is(L, t, "x"));
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "("));
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, ")"));
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "{"));
        t = lexer_next(L); assert(t.type == TK_IDENTIFIER); assert(token_is(L, t, "a"));
        lexer_pos(L, &col, &line); assert(col == 3 && line == 2);
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "=="));
        t = lexer_next(L); assert(t.type == TK_STRING); assert(token_is(L, t, "Hel\\\"lo"));

        char* str = token_extract(L, t); assert(strcmp(str, "Hel\\\"lo") == 0); free(str);

        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "+"));
        t = lexer_next(L); assert(t.type == TK_REAL); assert(fabs(t.v.r - 13.8) < 0.0000001);
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "*"));
        t = lexer_next(L); assert(t.type == TK_INTEGER); assert(t.type == TK_INTEGER && t.v.i == -0x12);
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, ";"));
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "}"));

        t = lexer_next(L); assert(t.type == TK_EOF);
        t = lexer_next(L); assert(t.type == TK_EOF);
        t = lexer_next(L); assert(t.type == TK_EOF);

        lexer_destroy(L);
    }

    {
        printf("## Lexer - sequence of symbols\n");

        Lexer* L = lexer_init("==()==");

        assert(lexer_peek(L).type == TK_NOT_STARTED);

        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "=="));
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "("));
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, ")"));
        t = lexer_next(L); assert(t.type == TK_SYMBOL); assert(token_is(L, t, "=="));
        t = lexer_next(L); assert(t.type == TK_EOF);

        lexer_destroy(L);
    }
}

static void assert_result(toml_table_t* item, TycheVM* T)
{
    toml_value_t t;

    const char* SS = "expected";
    if ((t = toml_table_int(item, SS)), t.ok) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == TYC_OK); assert(type == TYC_INTEGER);
        int32_t v; assert(tyc_tointeger(T, -1, &v) == TYC_OK); assert(v == t.u.i);
    } else if ((t = toml_table_double(item, SS)), t.ok) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == TYC_OK); assert(type == TYC_REAL);
        double v; assert(tyc_toreal(T, -1, &v) == TYC_OK); assert(fabs(v - t.u.d) < 0.0001);
    } else if ((t = toml_table_string(item, SS)), t.ok) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == TYC_OK); assert(type == TYC_STRING);
        const char* str; assert(tyc_tostring(T, -1, &str) == TYC_OK); assert(strcmp(str, t.u.s) == 0);
        free(t.u.s);
    } else if ((t = toml_table_bool(item, SS)), t.ok) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == TYC_OK); assert(type == TYC_BOOLEAN);
        bool v; assert(tyc_toboolean(T, -1, &v) == TYC_OK); assert(v == t.u.b);
    }
}

static void compile_and_run(toml_table_t* item, const char* code, bool debug, bool decompile)
{
    uint8_t* bytecode = NULL;
    size_t bytecode_sz = 0;
    char err[1024];

    Parser* P = parser_init(code);
    bool success = parser_compile(P, &bytecode, &bytecode_sz, err, sizeof err);
    assert(bytecode_sz > 0);

    if (success) {
        TycheVM* T = tyc_new();
        tyc_debug_to_console(T, debug);

        // run code
        assert(tyc_load_bytecode(T, bytecode, bytecode_sz) == TYC_OK);
        if (decompile)
            tyc_assembly_decompile(T);
        assert(tyc_call(T, 0) == TYC_OK);

        // assert
        assert_result(item, T);

        // cleanup
        free(bytecode);
        tyc_destroy(T);

    } else {
        fprintf(stderr, "compilation error: %s\n", err);
        free(err);
        assert(false);
    }
    parser_destroy(P);
}

static void run_test(const char* key, toml_table_t* item)
{
    toml_value_t t;
    printf("## %s\n", key);

    bool decompile = true;
    if ((t = toml_table_bool(item, "decompile")), t.ok)
        decompile = t.u.b;

    bool debug = false;
    if ((t = toml_table_bool(item, "debug")), t.ok)
        debug = t.u.b;

    if ((t = toml_table_string(item, "code")), t.ok) {
        compile_and_run(item, t.u.s, debug, decompile);
        free(t.u.s);
    }
}

static void test_parser()
{
    FILE* f = fopen("./test/test-code/compiler.toml", "r");
    if (!f) {
        fprintf(stderr, "Can't read TOML file.\n");
        exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buf = CALLOC(1, (size_t) size+1);
    (void) fread(buf, (size_t) size, 1, f);
    fclose(f);

    char errbuf[200];
    toml_table_t *tbl = toml_parse(buf, errbuf, sizeof(errbuf));
    if (!tbl) {
        fprintf(stderr, "ERROR: %s\n", errbuf);
        exit(1);
    }

    for (int i = 0; i < toml_table_len(tbl); i++) {
        int keylen;
        const char *key = toml_table_key(tbl, i, &keylen);
        toml_table_t *subtbl = toml_table_table(tbl, key);
        run_test(key, subtbl);
    }

    toml_free(tbl);
    free(buf);
}

//
// MAIN
//

int main(void)
{
    test_lexer();
    test_parser();
}