#include "../lib/assembler/assembler_priv.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_assembly(void)
{
    printf("## Test assembly\n");

    const char* assembly_code =
            ".const\n"
            "    0: 3.14\n"
            "    1: \"Hello world\"\n"
            "\n"
            ".func 0\n"
            "    pushi   2   ; this is a comment\n"
            "    pushi   -3\n"
            "    sum\n"
            "    ret\n"
            ".func 1\n"
            "    pushi   5000\n"
            "    ret";

    Assembly* assembly = assembly_new();
    assert(assemble(assembly_code, assembly) == T_OK);

    assert(assembly->consts_n == 2);
    assert(assembly->consts[0].type == TC_REAL);
    assert(assembly->consts[0].value.real > 3.13 && assembly->consts[0].value.real < 3.15);
    assert(assembly->consts[1].type == TC_STRING);
    assert(strcmp(assembly->consts[1].value.string, "Hello world") == 0);

    assert(assembly->functions_n == 2);
    assert(assembly->functions[0].n_instructions == 4);
    assert(assembly->functions[0].instructions[0].instruction == TO_PUSHI);
    assert(assembly->functions[0].instructions[0].operator.type == OP_INT);
    assert(assembly->functions[0].instructions[0].operator.v.i == 2);
    assert(assembly->functions[0].instructions[1].instruction == TO_PUSHI);
    assert(assembly->functions[0].instructions[1].operator.type == OP_INT);
    assert(assembly->functions[0].instructions[1].operator.v.i == -3);
    assert(assembly->functions[0].instructions[2].instruction == TO_SUM);
    assert(assembly->functions[0].instructions[3].instruction == TO_RET);

    assert(assembly->functions[1].n_instructions == 2);
    assert(assembly->functions[1].instructions[0].instruction == TO_PUSHI);
    assert(assembly->functions[1].instructions[0].operator.v.i == 5000);
    assert(assembly->functions[1].instructions[1].instruction == TO_RET);

    assembly_destroy(assembly);
}

static void test_labels(void)
{
    printf("## Test labels\n");

    const char* assembly_code =
            ".func 0\n"
            "    jmp    @my_label\n"
            "    pop   \n"
            "@my_label:\n"
            "    ret";

    Assembly* assembly = assembly_new();
    assert(assemble(assembly_code, assembly) == T_OK);

    assert(assembly->functions_n == 1);
    assert(assembly->functions[0].n_instructions == 3);
    assert(assembly->functions[0].instructions[0].instruction == TO_JMP);
    assert(assembly->functions[0].instructions[0].operator.type == OP_LABEL);
    assert(strcmp(assembly->functions[0].instructions[0].operator.v.label, "@my_label") == 0);
    assert(assembly->functions[0].instructions[1].instruction == TO_POP);
    assert(assembly->functions[0].instructions[2].n_labels == 1);
    assert(strcmp(assembly->functions[0].instructions[2].labels[0], "@my_label") == 0);
    assert(assembly->functions[0].instructions[2].instruction == TO_RET);

    // adjust labels

    assert(assembler_adjust_labels(assembly) == T_OK);

    assert(assembly->functions_n == 1);
    assert(assembly->functions[0].n_instructions == 3);
    assert(assembly->functions[0].instructions[0].instruction == TO_JMP);
    assert(assembly->functions[0].instructions[0].operator.type == OP_INT);
    assert(assembly->functions[0].instructions[0].operator.v.i == 4);

    assembly_destroy(assembly);
}

static void test_bytecode_gen(void)
{
    const char* assembly_code =
            ".const\n"
            "    0: 3.14\n"
            "    1: \"Hello\"\n"
            "\n"
            ".func 0\n"
            "    jmp @skip\n"
            "    pushi   2   ; this is a comment\n"
            "@skip:\n"
            "    ret\n"
            ".func 1\n"
            "    pushi   5000\n"
            "    ret";

    uint8_t bytecode_expected[] = {
        // header
        MAGIC, (MAGIC >> 8) & 0xff, (MAGIC >> 16) & 0xff, (MAGIC >> 24) & 0xff,   // magic number
        VERSION_MINOR, VERSION_MAJOR, 0x00, 0x00,                                   // version + reserved

        // constants
        0x21, 0x00, 0x00, 0x00,                 // code start address
        0x02, 0x00, 0x00, 0x00,                 // number of constants
        0x01, 0x1F, 0x85, 0xEB, 0x51, 0xB8, 0x1E, 0x09, 0x40,   // 3.14
        0x00, 'H', 'e', 'l', 'l', 'o', 0x00,                    // "Hello"

        // code
        0x00, 0x00, 0x00, 0x00,                 // debug start address
        0x02, 0x00, 0x00, 0x00,                 // number of functions
        0x33, 0x00, 0x00, 0x00,                 // address of function #1
        0xcc, 0x05, 0x00,           // jmp 5 (@skip)
        0xa0, 0x02,                 // pushi 2
        0x10,                       // ret
        0x00, 0x00, 0x00, 0x00,                 // address of function #1
        0xc0, 0x88, 0x13,           // pushi 500
        0x10,                       // ret
    };

    uint8_t* bytecode; size_t bytecode_sz;
    assert(code_assemble(assembly_code, &bytecode, &bytecode_sz) == T_OK);

    for (size_t i = 0; i < (bytecode_sz < sizeof bytecode_expected ? bytecode_sz : sizeof bytecode_expected); ++i) {
        printf("%02X|%02X ", bytecode[i], bytecode_expected[i]);
        if (i % 8 == 7)
            printf("\n");
    }
    printf("\n");

    assert(bytecode_sz == sizeof bytecode_expected);
    assert(memcmp(bytecode, bytecode_expected, bytecode_sz) == 0);
}

static void test_bytecode_parsing(void)
{
    printf("## Bytecode\n");
    const char* assembly_code =
            ".const\n"
            "    0: 3.14\n"
            "    1: \"Hello world\"\n"
            "\n"
            ".func 0\n"
            "    pushi   2   ; this is a comment\n"
            "    pushi   -3\n"
            "    sum\n"
            "    ret\n"
            ".func 1\n"
            "    pushi   5000\n"
            "    ret";

    uint8_t* bytecode; size_t bytecode_sz;
    assert(code_assemble(assembly_code, &bytecode, &bytecode_sz) == T_OK);

    Code* code = code_new();

    assert(code_load_bytecode(code, bytecode, bytecode_sz) == T_OK);

    assert(code_n_consts(code) == 2);
    assert(code_const_type(code, 0) == TC_REAL);
    assert(code_const_type(code, 1) == TC_STRING);
    assert(code_const_real(code, 0) > 3.13 && code_const_real(code, 0) < 3.15);
    assert(strcmp(code_const_string(code, 1), "Hello world") == 0);
    assert(code_n_functions(code) == 2);
    assert(code_function_sz(code, 0) == 6);
    assert(code_function_sz(code, 1) == 4);

    uint32_t addr = 0;
    Instruction inst = code_next_instruction(code, 0, addr);
    assert(inst.operator == TO_PUSHI);
    assert(inst.operand == 2);
    assert(inst.sz == 2);
    addr += inst.sz;

    inst = code_next_instruction(code, 0, addr);
    assert(inst.operator == TO_PUSHI);
    assert(inst.operand == -3);
    addr += inst.sz;

    inst = code_next_instruction(code, 0, addr);
    assert(inst.operator == TO_SUM);
    assert(inst.operand == 0);
    addr += inst.sz;

    inst = code_next_instruction(code, 1, 0);
    assert(inst.operator == TO_PUSHI + TO_16BIT);
    assert(inst.operand == 5000);
    assert(inst.sz == 3);

    code_destroy(code);
    free(bytecode);
}

static void test_bytecode_labels()
{
    printf("## Bytecode - labels\n");
    const char* assembly_code =
            ".func 0\n"
            "    jmp    @my_label\n"
            "    pushi  \n"
            "@my_label:\n"
            "    ret";

    uint8_t* bytecode; size_t bytecode_sz;
    assert(code_assemble(assembly_code, &bytecode, &bytecode_sz) == T_OK);

    Code* code = code_new();
    assert(code_load_bytecode(code, bytecode, bytecode_sz) == T_OK);

    Instruction inst = code_next_instruction(code, 0, 0);
    assert(inst.operator == TO_JMP);
    assert(inst.operand == 4);
    assert(inst.sz == 3);

    code_destroy(code);
    free(bytecode);
}

int main(void)
{
    test_assembly();
    test_labels();
    test_bytecode_gen();
    /*
    test_bytecode_parsing();
    test_bytecode_labels();
     */
}