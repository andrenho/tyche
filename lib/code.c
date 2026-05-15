#include "priv.h"

#include <stdlib.h>
#include <stdio.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  error Sorry, big endian architectures are not supported at this time.
#endif

#define MAGIC 0xa7d6e9b1

#define VERSION_ADDR        0x04
#define CODE_START_ADDR     0x08
#define N_CONST_ADDR        0x0c
#define CONST_START         0x10

struct Code {
    uint8_t*  bytecode;
    size_t    bytecode_sz;
    uint32_t* const_addr;
    uint32_t* fn_addr;
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

    for (size_t i = 0; i < bytecode_sz; ++i) {
        if (i % 16 == 0)
            printf("%04X: ", i);
        printf("%02x ", bytecode[i]);
        if (i % 16 == 15)
            printf("\n");
    }

    return T_OK;
}

size_t code_n_consts(Code const* code)
{
    uint32_t n_consts = *(uint32_t*) &code->bytecode[N_CONST_ADDR];
    return n_consts;
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
