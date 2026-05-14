#include "priv.h"

#include <stdlib.h>
#include <stdio.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  error Sorry, big endian architectures are not supported at this time.
#endif

#define MAGIC 0xa7d6e9b1

struct Code {
    uint8_t* bytecode;
    size_t   bytecode_sz;
};

Code* code_new()
{
    Code* code = xcalloc(1, sizeof(Code));
    return code;
}

void code_destroy(Code* code)
{
    free(code);
}

TYC_RESULT code_load_bytecode(Code* code, uint8_t* bytecode, size_t bytecode_sz)
{
    // TODO - linking

    if (bytecode_sz < 24)
        return T_ERR_BYTECODE_TOO_SMALL;

    uint32_t magic = *(uint32_t*) &bytecode[0];
    if (magic != MAGIC)
        return T_ERR_BYTECODE_INVALID_MAGIC;

    code->bytecode = bytecode;
    code->bytecode_sz = bytecode_sz;

    return T_OK;
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
