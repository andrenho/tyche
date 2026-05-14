#include "priv.h"

#include <stdlib.h>

struct Code {
};

Code* code_new(uint8_t* bytecode, size_t bytecode_sz)
{
    Code* code = xcalloc(1, sizeof(Code));
    return code;
}

void code_destroy(Code* code)
{
    free(code);
}

size_t code_n_consts(Code const* code)
{
    return 0;  // TODO
}

TYC_CONST_TYPE code_const_type(Code const* code, size_t n)
{
    return TC_REAL;  // TODO
}

T_REAL code_const_real(Code const* code, size_t n)
{
    return 0.f;  // TODO
}

const char* code_const_string(Code const* code, size_t n)
{
    return "";  // TODO
}

size_t code_n_functions(Code const* code)
{
    return 0;  // TODO
}
