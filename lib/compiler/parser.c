#include "compiler_priv.h"

struct Parser {
    Lexer* L;
};

Parser* parser_init(const char* code)
{
    Parser* P = xcalloc(1, sizeof(Parser));
    P->L = lexer_init(code);
    return P;
}

void parser_destroy(Parser* P)
{
    lexer_destroy(P->L);
    free(P);
}

bool parser_compile(Parser* P, uint8_t** bytecode_out, size_t* bytecode_sz_out, char* err, size_t err_sz)
{
    Assembly* ass = assembly_new();


    int r = assembler_adjust_labels(ass);
    if (r != TYC_OK) {
        snprintf(err, err_sz, "assembly error adjusting labels");
        return false;
    }
    r = bytecode_gen(ass, bytecode_out, bytecode_sz_out);
    if (r != TYC_OK) {
        snprintf(err, err_sz, "assembly error");
        return false;
    }
    assembly_destroy(ass);

    return true;
}