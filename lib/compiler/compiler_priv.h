#ifndef TYCHE_COMPILER_PRIV_H
#define TYCHE_COMPILER_PRIV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../assembler/assembler_priv.h"

//
// LEXER
//

typedef struct {
    size_t start;
    size_t end;
} TkStringRef;

typedef enum {
    TK_NOT_STARTED, TK_EOF, TK_INTEGER, TK_REAL, TK_IDENTIFIER, TK_STRING, TK_SYMBOL, TK_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    union {
        TkStringRef s;
        int32_t     i;
        double      r;
    } v;
} Token;

typedef struct Lexer Lexer;

Lexer* lexer_init(const char* code);   // don't free code during execution
void   lexer_destroy(Lexer* L);

Token  lexer_peek(Lexer* L);
Token  lexer_next(Lexer* L);

void   lexer_pos(Lexer const* L, int* column, int* line);

bool   token_is(Lexer const* L, Token t, const char* cmp);
char*  token_extract(Lexer const* L, Token t);  // returned string needs to be freed

//
// PARSER
//

typedef struct Parser Parser;

Parser* parser_init(const char* code);  // don't free code during parsing
void    parser_destroy(Parser* P);

bool    parser_compile(Parser* P, uint8_t** bytecode_out, size_t* bytecode_sz_out, char* err, size_t err_sz);

#endif //TYCHE_COMPILER_PRIV_H
