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

typedef enum { OP_NONE, OP_INT, OP_LABEL } OperatorType;

typedef struct {
    OperatorType type;
    union {
        int32_t i;
        char*   label;
    } v;
} AssemblyOperator;

typedef struct {
    TYC_INST         instruction;
    AssemblyOperator operator;
    char**           labels;
    size_t           n_labels;
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

Assembly* assembly_new(void);
void      assembly_destroy(Assembly* as);

void      assembly_add_const_str(Assembly* as, uint32_t c_id, const char* value);
void      assembly_add_const_real(Assembly* as, uint32_t c_id, double value);
void      assembly_add_function(Assembly* as, uint32_t f_id);
void      assembly_add_inst(Assembly* as, uint32_t f_id, TYC_INST inst);
void      assembly_add_inst_p(Assembly* as, uint32_t f_id, TYC_INST inst, int32_t par);

#endif //TYCHE_COMPILER_PRIV_H
