extern "C" {
#include "../lib/assembler/assembler_priv.h"
}

#include <gtest/gtest.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

__thread char last_err_msg[256] = {0};

TEST(Assembly, Compilation)
{
    const char* assembly_code = R"(.assembly
        .const
            0: 3.14
            1: "Hello world"

        .func 0
            pushi   2   ; this is a comment
            pushi   -3
            sum
            ret
        .func 1
            pushi   5000
            ret
    )";

    Assembly* assembly = assembly_new();
    EXPECT_EQ(assemble(assembly_code, assembly), TYC_OK);

    EXPECT_EQ(assembly->consts_n, 2);
    EXPECT_EQ(assembly->consts[0].type, TC_REAL);
    EXPECT_DOUBLE_EQ(assembly->consts[0].value.real, 3.14);
    EXPECT_EQ(assembly->consts[1].type, TC_STRING);
    EXPECT_EQ(strcmp(assembly->consts[1].value.string, "Hello world"), 0);

    EXPECT_EQ(assembly->functions_n, 2);
    EXPECT_EQ(assembly->functions[0].n_instructions, 4);
    EXPECT_EQ(assembly->functions[0].instructions[0].instruction, TO_PUSHI);
    EXPECT_EQ(assembly->functions[0].instructions[0].operator_.type, OP_INT);
    EXPECT_EQ(assembly->functions[0].instructions[0].operator_.v.i, 2);
    EXPECT_EQ(assembly->functions[0].instructions[1].instruction, TO_PUSHI);
    EXPECT_EQ(assembly->functions[0].instructions[1].operator_.type, OP_INT);
    EXPECT_EQ(assembly->functions[0].instructions[1].operator_.v.i, -3);
    EXPECT_EQ(assembly->functions[0].instructions[2].instruction, TO_SUM);
    EXPECT_EQ(assembly->functions[0].instructions[3].instruction, TO_RET);

    EXPECT_EQ(assembly->functions[1].n_instructions, 2);
    EXPECT_EQ(assembly->functions[1].instructions[0].instruction, TO_PUSHI);
    EXPECT_EQ(assembly->functions[1].instructions[0].operator_.v.i, 5000);
    EXPECT_EQ(assembly->functions[1].instructions[1].instruction, TO_RET);

    assembly_destroy(assembly);
}

TEST(Assembly, Labels)
{
    const char* assembly_code =
            ".assembly\n"
            ".func 0\n"
            "    jmp    @my_label\n"
            "    pop   \n"
            "@my_label:\n"
            "    ret";

    Assembly* assembly = assembly_new();
    EXPECT_EQ(assemble(assembly_code, assembly), TYC_OK);

    EXPECT_EQ(assembly->functions_n, 1);
    EXPECT_EQ(assembly->functions[0].n_instructions, 3);
    EXPECT_EQ(assembly->functions[0].instructions[0].instruction, TO_JMP);
    EXPECT_EQ(assembly->functions[0].instructions[0].operator_.type, OP_LABEL);
    EXPECT_EQ(strcmp(assembly->functions[0].instructions[0].operator_.v.label, "@my_label"), 0);
    EXPECT_EQ(assembly->functions[0].instructions[1].instruction, TO_POP);
    EXPECT_EQ(assembly->functions[0].instructions[2].n_labels, 1);
    EXPECT_EQ(strcmp(assembly->functions[0].instructions[2].labels[0], "@my_label"), 0);
    EXPECT_EQ(assembly->functions[0].instructions[2].instruction, TO_RET);

    // adjust labels

    EXPECT_EQ(assembler_adjust_labels(assembly), TYC_OK);

    EXPECT_EQ(assembly->functions_n, 1);
    EXPECT_EQ(assembly->functions[0].n_instructions, 3);
    EXPECT_EQ(assembly->functions[0].instructions[0].instruction, TO_JMP);
    EXPECT_EQ(assembly->functions[0].instructions[0].operator_.type, OP_INT);
    EXPECT_EQ(assembly->functions[0].instructions[0].operator_.v.i, 4);

    assembly_destroy(assembly);
}

TEST(Bytecode, Generation)
{
    const char* assembly_code =
            ".assembly\n"
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
        (uint8_t) MAGIC, (MAGIC >> 8) & 0xff, (MAGIC >> 16) & 0xff, (MAGIC >> 24) & 0xff,   // magic number
        VERSION_MINOR, VERSION_MAJOR, 0x00, 0x00,                                   // version + reserved

        // constants
        0x20, 0x00, 0x00, 0x00,                 // code start address
        0x02, 0x00, 0x00, 0x00,                 // number of constants
        0x01, 0x1F, 0x85, 0xEB, 0x51, 0xB8, 0x1E, 0x09, 0x40,   // 3.14
        0x00, 'H', 'e', 'l', 'l', 'o', 0x00,                    // "Hello"

        // code
        0x00, 0x00, 0x00, 0x00,                 // debug start address
        0x02, 0x00, 0x00, 0x00,                 // number of functions
        0x32, 0x00, 0x00, 0x00,                 // address of function #1
        0xcc, 0x05, 0x00,           // jmp 5 (@skip)
        0xa0, 0x02,                 // pushi 2
        0x10,                       // ret
        0x3a, 0x00, 0x00, 0x00,                 // address of function #1
        0xc0, 0x88, 0x13,           // pushi 500
        0x10,                       // ret
    };

    uint8_t* bytecode; size_t bytecode_sz;
    EXPECT_EQ(code_assemble(assembly_code, &bytecode, &bytecode_sz), TYC_OK);

    /*
    for (size_t i = 0; i < (bytecode_sz < sizeof bytecode_expected ? bytecode_sz : sizeof bytecode_expected); ++i) {
        printf("%02X|%02X ", bytecode[i], bytecode_expected[i]);
        if (i % 8 == 7)
            printf("\n");
    }
    printf("\n");
    */

    EXPECT_EQ(bytecode_sz, sizeof bytecode_expected);
    EXPECT_EQ(memcmp(bytecode, bytecode_expected, bytecode_sz), 0);

    free(bytecode);
}

TEST(Bytecode, Parsing)
{
    const char* assembly_code =
            ".assembly\n"
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
    EXPECT_EQ(code_assemble(assembly_code, &bytecode, &bytecode_sz), TYC_OK);

    /*
    for (size_t i = 0; i < bytecode_sz; ++i) {
        printf("%02X ", bytecode[i]);
        if (i % 8 == 7)
            printf("\n");
    }
    printf("\n");
    */

    Code* code = code_new();

    EXPECT_EQ(code_load_bytecode(code, bytecode, bytecode_sz), TYC_OK);

    EXPECT_EQ(code_n_consts(code), 2);
    EXPECT_EQ(code_const_type(code, 0), TC_REAL);
    EXPECT_EQ(code_const_type(code, 1), TC_STRING);
    EXPECT_DOUBLE_EQ(code_const_real(code, 0), 3.14);
    EXPECT_EQ(strcmp(code_const_string(code, 1), "Hello world"), 0);
    EXPECT_EQ(code_n_functions(code), 2);
    EXPECT_EQ(code_function_sz(code, 0), 6);
    EXPECT_EQ(code_function_sz(code, 1), 4);

    uint32_t addr = 0;
    Instruction inst = code_next_instruction(code, 0, addr);
    EXPECT_EQ(inst.operator_, TO_PUSHI);
    EXPECT_EQ(inst.operand, 2);
    EXPECT_EQ(inst.sz, 2);
    addr += inst.sz;

    inst = code_next_instruction(code, 0, addr);
    EXPECT_EQ(inst.operator_, TO_PUSHI);
    EXPECT_EQ(inst.operand, -3);
    addr += inst.sz;

    inst = code_next_instruction(code, 0, addr);
    EXPECT_EQ(inst.operator_, TO_SUM);
    EXPECT_EQ(inst.operand, 0);
    addr += inst.sz;

    inst = code_next_instruction(code, 1, 0);
    EXPECT_EQ(inst.operator_, TO_PUSHI + TO_16BIT);
    EXPECT_EQ(inst.operand, 5000);
    EXPECT_EQ(inst.sz, 3);

    code_destroy(code);
    free(bytecode);
}

TEST(Bytecode, Labels)
{
    const char* assembly_code =
            ".assembly\n"
            ".func 0\n"
            "    jmp    @my_label\n"
            "    sum  \n"
            "@my_label:\n"
            "    ret";

    uint8_t* bytecode; size_t bytecode_sz;
    EXPECT_EQ(code_assemble(assembly_code, &bytecode, &bytecode_sz), TYC_OK);

    Code* code = code_new();
    EXPECT_EQ(code_load_bytecode(code, bytecode, bytecode_sz), TYC_OK);

    Instruction inst = code_next_instruction(code, 0, 0);
    EXPECT_EQ(inst.operator_, TO_JMP);
    EXPECT_EQ(inst.operand, 4);
    EXPECT_EQ(inst.sz, 3);

    code_destroy(code);
    free(bytecode);
}