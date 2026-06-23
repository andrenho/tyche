#include "compiler_priv.h"
#include "priv.h"

#include <ctype.h>
#include <errno.h>

#define LEXER_ERROR_SZ 256

struct Lexer {
    const char* code;
    const char* cur;
    const char* end;
    char        lexer_error[LEXER_ERROR_SZ];
    int         column, line;
    int         prev_column, prev_line;
    Token       next_token;
};

static const char* TWO_CHAR_SYMBOLS[] = { "==" };
static const char SINGLE_CHAR_SYMBOLS[] = { '(', ')', '+', '*', ';', '{', '}' };

static void advance(Lexer* L)
{
    if (*L->cur == '\n') {
        ++L->line;
        L->column = 0;
    }
    ++L->cur;
    ++L->column;
}

static void err_invalid_char(Lexer* L)
{
    L->next_token.type = TK_ERROR;
    snprintf(L->lexer_error, LEXER_ERROR_SZ, "invalid token in line %d, column %d\n", L->line, L->column);
}

static void get_next_token(Lexer* L)
{
again:
    L->prev_column = L->column;
    L->prev_line = L->line;

    if (L->next_token.type == TK_ERROR)
        return;

    // check for EOF
    if (L->cur >= L->end) {
        L->next_token.type = TK_EOF;
        return;
    }

    // ignore spaces
    if (isspace(*L->cur)) {
        advance(L);
        goto again;
    }

    // ignore comments
    if (L->cur < L->end - 1 && strncmp(L->cur, "//", 2) == 0) {
        while (*L->cur != '\n')
            advance(L);
        goto again;
        return;
    }

    // first char is a number
    if (isdigit(*L->cur) || *L->cur == '.' || *L->cur == '-') {
        const char* start = L->cur;
        char* end;

        long lvalue = strtol(start, &end, 0);

        if (end + 1 < L->end && end[0] == '.') {  // check if double
            double dvalue = strtod(start, &end);
            if (start == end || errno == ERANGE)
                err_invalid_char(L);
            L->next_token.type = TK_REAL;
            L->next_token.v.r = dvalue;
            L->cur = end;
        } else {
            if (start == end || errno == ERANGE)
                err_invalid_char(L);
            L->next_token.type = TK_INTEGER;
            L->next_token.v.i = (int32_t) lvalue;   // TODO - what if integer value is too large?
            L->cur = end;
        }

    // first char is a letter
    } else if (isalpha(*L->cur) || *L->cur == '_') {
        L->next_token.type = TK_IDENTIFIER;
        L->next_token.v.s.start = L->next_token.v.s.end = (size_t) (L->cur - L->code);
        while (isalpha(*L->cur) || *L->cur == '_') {
            ++L->next_token.v.s.end;
            advance(L);
        }

    // first char is quote
    } else if (*L->cur == '"') {
        advance(L);

        L->next_token.type = TK_STRING;
        L->next_token.v.s.start = L->next_token.v.s.end = (size_t) (L->cur - L->code);

        while (L->cur < L->end && *L->cur != '"') {
            if (*L->cur == '\\') {
                advance(L);
                ++L->next_token.v.s.end;
            }
            advance(L);
            ++L->next_token.v.s.end;
        }

        advance(L);

    // first char is a symbol
    } else if (ispunct(*L->cur)) {
        L->next_token.type = TK_SYMBOL;
        L->next_token.v.s.start = (size_t) (L->cur - L->code);
        L->next_token.v.s.end = L->next_token.v.s.start + 1;

        // check for two-char symbols
        if (L->cur < L->end - 1) {
            for (size_t i = 0; i < (sizeof TWO_CHAR_SYMBOLS / sizeof TWO_CHAR_SYMBOLS[0]); ++i) {
                if (strncmp(L->cur, TWO_CHAR_SYMBOLS[i], 2) == 0) {
                    ++L->next_token.v.s.end;
                    advance(L); advance(L);
                    return;
                }
            }
        }

        // check for single-char symbol
        for (size_t i = 0; i < sizeof(SINGLE_CHAR_SYMBOLS); ++i) {
            if (*L->cur == SINGLE_CHAR_SYMBOLS[i]) {
                advance(L);
                return;
            }
        }

        err_invalid_char(L);

    // invalid token
    } else {
        err_invalid_char(L);
    }
}

Lexer* lexer_init(const char* code)
{
    Lexer* L = xcalloc(1, sizeof(Lexer));
    L->code = code;
    L->cur = L->code;
    L->end = L->code + strlen(L->code);
    L->next_token.type = TK_NOT_STARTED;
    L->column = L->line = L->prev_column = L->prev_line = 1;
    return L;
}

void lexer_destroy(Lexer* L)
{
    free(L);
}

Token lexer_peek(Lexer* L)
{
    return L->next_token;
}

Token lexer_next(Lexer* L)
{
    get_next_token(L);
    return L->next_token;
}

bool token_is(Lexer const* L, Token t, const char* cmp)
{
    assert(t.type == TK_IDENTIFIER || t.type == TK_STRING || t.type == TK_SYMBOL);
    return strncmp(&L->code[t.v.s.start], cmp, t.v.s.end - t.v.s.start) == 0;
}

char* token_extract(Lexer const* L, Token t)
{
    char* str = xcalloc(t.v.s.end - t.v.s.start + 1, 1);
    memcpy(str, &L->code[t.v.s.start], t.v.s.end - t.v.s.start);
    return str;
}

void lexer_pos(Lexer const* L, int* column, int* line)
{
    if (column) *column = L->prev_column;
    if (line) *line = L->prev_line;
}