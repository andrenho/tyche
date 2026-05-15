#include "priv.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  error Sorry, big endian architectures are not supported at this time.
#endif

#define MAGIC 0xa7d6e9b1

#define VERSION_ADDR        0x04
#define CODE_START_ADDR     0x08
#define N_CONST_ADDR        0x0c
#define CONST_START         0x10

struct Code {
    uint8_t const* bytecode;
    size_t         bytecode_sz;
    uint32_t*      const_addr;
    uint32_t       fn_count;
    uint32_t*      fn_addr;
};

Code* code_new(void)
{
    Code* code = xcalloc(1, sizeof(Code));
    return code;
}

void code_destroy(Code* code)
{
    free(code->const_addr);
    free(code->fn_addr);
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

    /*
    for (size_t i = 0; i < bytecode_sz; ++i) {
        if (i % 16 == 0)
            printf("%04X: ", i);
        printf("%02x ", bytecode[i]);
        if (i % 16 == 15)
            printf("\n");
    }
    printf("\n");
    */

    uint32_t n_consts = code_n_consts(code);
    code->const_addr = calloc(n_consts, sizeof(uint32_t));
    uint32_t addr = CONST_START;
    for (size_t i = 0; i < n_consts; ++i) {
        code->const_addr[i] = addr;
        switch (code_const_type(code, i)) {
            case TC_STRING: {
                uint32_t sz = (uint32_t) strlen((const char*) &bytecode[code->const_addr[i] + 1]);
                addr += sz + 2;  // 2 = constant type + NULL terminator
                break;
            }
            case TC_REAL:
                addr += 5;  // 5 = constant type + float
                break;
            case TC_INVALID_TYPE:
            default:
                __builtin_unreachable();
        }
    }

    addr += 4;  // skip debug start address
    memcpy(&code->fn_count, &bytecode[addr], sizeof(uint32_t));  // number of functions
    addr += 4;

    code->fn_addr = calloc(code->fn_count, sizeof(uint32_t));
    code->fn_addr[0] = addr;
    for (size_t i = 1; i < code->fn_count; ++i) {
        uint32_t addr_next;
        memcpy(&addr_next, &bytecode[addr], sizeof(uint32_t));
        addr = code->fn_addr[i] = addr_next;
    }

    return T_OK;
}

uint32_t code_n_consts(Code const* code)
{
    uint32_t n_consts; memcpy(&n_consts, &code->bytecode[N_CONST_ADDR], sizeof(uint32_t));
    return n_consts;
}

TYC_CONST_TYPE code_const_type(Code const* code, size_t n)
{
    uint8_t t = code->bytecode[code->const_addr[n]];
    if (t >= TC_INVALID_TYPE)
        return TC_INVALID_TYPE;
    return t;
}

T_REAL code_const_real(Code const* code, size_t n)
{
    float f;
    memcpy(&f, &code->bytecode[code->const_addr[n] + 1], sizeof(float));
    return f;
}

const char* code_const_string(Code const* code, size_t n)
{
    return (const char*) &code->bytecode[code->const_addr[n] + 1];
}

uint32_t code_n_functions(Code const* code)
{
    return code->fn_count;
}
