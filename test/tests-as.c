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

    assembly_destroy(assembly);
}

// TODO - adjust labels

// TODO - bytecode generation