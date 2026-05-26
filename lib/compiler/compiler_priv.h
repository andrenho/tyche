#ifndef TYCHE_COMPILER_PRIV_H
#define TYCHE_COMPILER_PRIV_H

#include "../priv.h"

typedef struct {
    TYC_CONST_TYPE type;
    union {
        char*  string;
        T_REAL real;
    } value;
} AssemblyConst;

typedef struct {
    Instruction instruction;
    int32_t     operator;
    char**      labels;
    size_t      n_labels;
} AssemblyInstruction;

typedef struct {
    AssemblyInstruction* instructions;
    size_t               n_instructions;
} AssemblyFunction;

struct Assembly {
    AssemblyConst*    consts;
    size_t            consts_n;
    AssemblyFunction* functions;
    size_t            functions_n;
    char*             error;
};

Assembly* assembly_new();
void      assembly_destroy(Assembly* assembly);

#endif //TYCHE_COMPILER_PRIV_H
