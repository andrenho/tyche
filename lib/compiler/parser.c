#include "compiler_priv.h"

typedef struct Function {
} Function;

struct Parser {
    Lexer*    L;
    Assembly* A;
    size_t*   func_stack;
    size_t    func_stack_sz;
};

//
// PARSING EVENTS
//

static size_t start_function(Parser* P)
{
    ++P->func_stack_sz;
}

static void end_function(Parser* P)
{
}


//
// PARSING
//

static void statements(Parser* P)
{
}


//
// PARSER MANAGEMENT
//

Parser* parser_init(const char* code)
{
    Parser* P = xcalloc(1, sizeof(Parser));
    P->L = lexer_init(code);
    P->A = assembly_new();
    P->func_stack = NULL;
    P->func_stack_sz = 0;
    return P;
}

void parser_destroy(Parser* P)
{
    assembly_destroy(P->A);
    lexer_destroy(P->L);
    free(P);
}

bool parser_compile(Parser* P, uint8_t** bytecode_out, size_t* bytecode_sz_out, char* err, size_t err_sz)
{
    statements(P);
    end_function(P);

    // generate bytecode
    int r = assembler_adjust_labels(P->A);
    if (r != TYC_OK) {
        snprintf(err, err_sz, "assembly error adjusting labels");
        return false;
    }
    r = bytecode_gen(P->A, bytecode_out, bytecode_sz_out);
    if (r != TYC_OK) {
        snprintf(err, err_sz, "assembly error");
        return false;
    }

    return true;
}