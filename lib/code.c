#include "priv.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "instructions/instructions.h"

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  error Sorry, big endian architectures are not supported at this time.
#endif

#define MAGIC 0xa7d6e9b1

#define VERSION_ADDR        0x04
#define CODE_START_ADDR     0x08
#define N_CONST_ADDR        0x0c
#define CONST_START         0x10

#define OP_8BIT_OPERAND  0xa0
#define OP_16BIT_OPERAND 0xc0
#define OP_32BIT_OPERAND 0xe0

struct Code {
    uint8_t const* bytecode;
    size_t         bytecode_sz;
    uint32_t*      const_addr;
    uint32_t       fn_count;
    uint32_t*      fn_addr;
    uint32_t*      fn_sz;
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
    free(code->fn_sz);
    free(code);
}

TYC_RESULT code_load_bytecode(Code* code, uint8_t const* bytecode, size_t bytecode_sz)
{
    // TODO - linking

    if (bytecode_sz < 24)
        ERROR("Bytecode: too small")

    uint32_t magic;
    memcpy(&magic, bytecode, sizeof(magic));
    if (magic != MAGIC)
        ERROR("Bytecode: invalid magic number")

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
    code->const_addr = xcalloc(n_consts, sizeof(uint32_t));
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
                addr += 9;  // 9 = constant type + double
                break;
            case TC_INVALID_TYPE:
            default:
                __builtin_unreachable();
        }
    }

    addr += 4;  // skip debug start address
    memcpy(&code->fn_count, &bytecode[addr], sizeof(uint32_t));  // number of functions
    addr += 4;

    code->fn_addr = xcalloc(code->fn_count, sizeof(uint32_t));
    code->fn_sz = xcalloc(code->fn_count, sizeof(uint32_t));
    code->fn_addr[0] = addr;
    uint32_t addr_next;
    for (size_t i = 1; i < code->fn_count; ++i) {
        memcpy(&addr_next, &bytecode[addr], sizeof(uint32_t));
        code->fn_sz[i-1] = addr_next - addr - 4;
        addr = code->fn_addr[i] = addr_next;
    }
    memcpy(&addr_next, &bytecode[addr], sizeof(uint32_t));
    code->fn_sz[code->fn_count-1] = addr_next - addr - 4;

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
    T_REAL f;
    memcpy(&f, &code->bytecode[code->const_addr[n] + 1], sizeof(T_REAL));
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

uint32_t code_function_sz(Code const* code, uint32_t f_id)
{
    return code->fn_sz[f_id];
}

Instruction code_next_instruction(Code const* code, uint32_t function_id, uint32_t pc)
{
    uint32_t addr = code->fn_addr[function_id] + 4 + pc;
    uint8_t opcode = code->bytecode[addr];
    int32_t operand = 0;
    uint8_t sz = 1;

    if (opcode >= OP_8BIT_OPERAND && opcode < OP_16BIT_OPERAND) {
        operand = (int8_t) code->bytecode[addr + 1];
        sz = 2;
    } else if (opcode >= OP_16BIT_OPERAND && opcode < OP_32BIT_OPERAND) {
        // opcode -= 0x20;
        operand = (int16_t) ((uint16_t) code->bytecode[addr + 1] |
                             (uint16_t) (code->bytecode[addr + 2] << 8));
        sz = 3;
    } else if (opcode >= OP_32BIT_OPERAND) {
        // opcode -= 0x40;
        operand = (int32_t) ((uint32_t) code->bytecode[addr + 1] |
                             (uint32_t) (code->bytecode[addr + 2] << 8) |
                             (uint32_t) (code->bytecode[addr + 3] << 16) |
                             (uint32_t) (code->bytecode[addr + 4] << 24));
        sz = 5;
    }

    return (Instruction) {
        .operator = (TYC_INST) opcode,
        .operand = operand,
        .sz = sz,
    };
}

#ifdef DEBUG_ASSEMBLY

void code_debug_bytecode(Code const* code)
{
    for (int i = 0; i < (int) code->bytecode_sz; ++i) {
        if (i % 16 == 0)
            printf("%04X : ", i);
        printf("%02X ", code->bytecode[i]);
        if (i % 16 == 15)
            printf("\n");
    }
    printf("\n");
}

void code_decompile(Code const* code)
{
    if (code_n_consts(code) > 0)
        printf(".const\n");

    for (size_t const_id = 0; const_id < code_n_consts(code); ++const_id) {
        TYC_CONST_TYPE type = code_const_type(code, const_id);
        if (type == TC_STRING)
            printf("  %03zu: \"%s\"\n", const_id, code_const_string(code, const_id));
        else if (type == TC_REAL)
            printf("  %03zu: %f\n", const_id, code_const_real(code, const_id));
    }

    for (uint32_t f_id = 0; f_id < code_n_functions(code); ++f_id) {
        printf(".func %d\n", f_id);
        uint32_t pc = 0;
        while (pc < code_function_sz(code, f_id)) {
            Instruction inst = code_next_instruction(code, f_id, pc);
            char buf[50];
            code_parse_instruction(inst, buf, sizeof buf);
            printf("    %s  ; %d\n", buf, pc);
            pc += inst.sz;
        }
    }
}

void code_parse_instruction(Instruction inst, char* outbuf, size_t sz)
{
    int n = snprintf(outbuf, sz, "%s", instruction_name(inst.operator));

    if (inst.operator >= OP_8BIT_OPERAND)
        snprintf(&outbuf[n], sz + (size_t) n, "%2d", inst.operand);
    else
        snprintf(&outbuf[n], sz + (size_t) n, "  ");
}

#endif