#include "../lib/compiler/compiler_priv.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void)
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
    assert(assembly->functions[0].instructions[0].operator == 2);
    assert(assembly->functions[0].instructions[1].instruction == TO_PUSHI);
    assert(assembly->functions[0].instructions[1].operator == -3);
    assert(assembly->functions[0].instructions[2].instruction == TO_SUM);
    assert(assembly->functions[0].instructions[3].instruction == TO_RET);

    assert(assembly->functions[1].n_instructions == 2);
    assert(assembly->functions[1].instructions[0].instruction == TO_PUSHI);
    assert(assembly->functions[1].instructions[0].operator == 5000);
    assert(assembly->functions[1].instructions[1].instruction == TO_RET);

    assembly_destroy(assembly);
}

// TODO - adjust labels

// TODO - bytecode generation