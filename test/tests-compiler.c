#include "../lib/compiler/compiler_priv.h"
#include "../lib/priv.h"

#include <assert.h>
#include <math.h>
#include <string.h>

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

//
// MAIN
//

int main(void)
{
    test_lexer();
}