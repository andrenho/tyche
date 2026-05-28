#include "compiler_priv.h"

#include <stdlib.h>

Assembly* assembly_new(void)
{
    return xcalloc(1, sizeof(Assembly));
}


void assembly_destroy(Assembly* as)
{
    for (size_t i = 0; i < as->consts_n; ++i)
        if (as->consts[i].type == TC_STRING)
            free(as->consts[i].value.string);
    free(as->consts);

    for (size_t i = 0; i < as->functions_n; ++i) {
        for (size_t j = 0; j < as->functions[i].n_instructions; ++j)
            for (size_t k = 0; k < as->functions[i].instructions[j].n_labels; ++k)
                free(as->functions[i].instructions[j].labels[k]);
        free(as->functions[i].instructions);
    }
    free(as->functions);

    free(as->error);
    free(as);
}

void assembly_add_const_str(Assembly* as, uint32_t c_id, const char* value)
{
    if (c_id + 1 > as->consts_n) {
        as->consts_n = c_id + 1;
        as->consts = xrealloc(as->consts, (c_id + 1) * sizeof(AssemblyConst));
    }
    as->consts[c_id] = (AssemblyConst) { .type = TC_STRING, .value.string = strdup(value) };
}

void assembly_add_const_real(Assembly* as, uint32_t c_id, double value)
{
    if (c_id + 1 > as->consts_n) {
        as->consts_n = c_id + 1;
        as->consts = xrealloc(as->consts, (c_id + 1) * sizeof(AssemblyConst));
    }
    as->consts[c_id] = (AssemblyConst) { .type = TC_REAL, .value.real = value };
}

void assembly_add_function(Assembly* as, uint32_t f_id)
{
    if (f_id + 1 > as->functions_n) {
        as->functions_n = f_id + 1;
        as->functions = xrealloc(as->functions, (f_id + 1) * sizeof(AssemblyFunction));
    }
    as->functions[f_id] = (AssemblyFunction) { .instructions = NULL, .n_instructions = 0 };
}

void assembly_add_inst(Assembly* as, uint32_t f_id, TYC_INST inst)
{
    ++as->functions[f_id].n_instructions;
    as->functions[f_id].instructions = xrealloc(as->functions[f_id].instructions, as->functions[f_id].n_instructions * sizeof(AssemblyInstruction));
    as->functions[f_id].instructions[as->functions[f_id].n_instructions - 1] = (AssemblyInstruction) {
        .instruction = inst,
        .operator = { .type = OP_NONE },
        .labels = NULL,
        .n_labels = 0,
    };
}

void assembly_add_inst_p(Assembly* as, uint32_t f_id, TYC_INST inst, int32_t par)
{
    ++as->functions[f_id].n_instructions;
    as->functions[f_id].instructions = xrealloc(as->functions[f_id].instructions, as->functions[f_id].n_instructions * sizeof(AssemblyInstruction));
    as->functions[f_id].instructions[as->functions[f_id].n_instructions - 1] = (AssemblyInstruction) {
        .instruction = inst,
        .operator = { .type = OP_INT, .v.i = par },
        .labels = NULL,
        .n_labels = 0,
    };
}
