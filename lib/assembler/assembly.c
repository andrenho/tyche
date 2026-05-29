#include "assembler_priv.h"

#include <stdlib.h>

struct AssemblyPriv {
    char** next_labels;
    size_t n_next_labels;
};

Assembly* assembly_new(void)
{
    Assembly* as = xcalloc(1, sizeof(Assembly));
    as->priv_ = xcalloc(1, sizeof(AssemblyPriv));
    return as;
}

void assembly_destroy(Assembly* as)
{
    for (size_t i = 0; i < as->consts_n; ++i)
        if (as->consts[i].type == TC_STRING)
            free(as->consts[i].value.string);
    free(as->consts);

    for (size_t i = 0; i < as->functions_n; ++i) {
        for (size_t j = 0; j < as->functions[i].n_instructions; ++j) {
            if (as->functions[i].instructions[j].operator.type == OP_LABEL)
                free(as->functions[i].instructions[j].operator.v.label);
            for (size_t k = 0; k < as->functions[i].instructions[j].n_labels; ++k)
                free(as->functions[i].instructions[j].labels[k]);
            free(as->functions[i].instructions[j].labels);
        }
        free(as->functions[i].instructions);
    }
    free(as->functions);

    for (size_t i = 0; i < as->priv_->n_next_labels; ++i)
        free(as->priv_->next_labels[i]);
    free(as->priv_);

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

static void assembly_add_instruction(Assembly* as, uint32_t f_id, AssemblyInstruction instruction)
{
    ++as->functions[f_id].n_instructions;
    as->functions[f_id].instructions = xrealloc(as->functions[f_id].instructions, as->functions[f_id].n_instructions * sizeof(AssemblyInstruction));
    as->functions[f_id].instructions[as->functions[f_id].n_instructions - 1] = instruction;
    as->functions[f_id].instructions[as->functions[f_id].n_instructions - 1].n_labels = as->priv_->n_next_labels;
    as->functions[f_id].instructions[as->functions[f_id].n_instructions - 1].labels = as->priv_->next_labels;

    as->priv_->n_next_labels = 0;
    as->priv_->next_labels = NULL;
}

void assembly_add_inst(Assembly* as, uint32_t f_id, TYC_INST inst)
{
    assembly_add_instruction(as, f_id, (AssemblyInstruction) {
        .instruction = inst,
        .operator = { .type = OP_NONE },
    });
}

void assembly_add_inst_p(Assembly* as, uint32_t f_id, TYC_INST inst, int32_t par)
{
    assembly_add_instruction(as, f_id, (AssemblyInstruction) {
        .instruction = inst,
        .operator = { .type = OP_INT, .v.i = par },
    });
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"
void assembly_add_inst_label(Assembly* as, uint32_t f_id, TYC_INST inst, const char* label)
{
    assembly_add_instruction(as, f_id, (AssemblyInstruction) {
        .instruction = inst,
        .operator = { .type = OP_LABEL, .v.label = strdup(label) },
    });
}
#pragma GCC diagnostic pop

void assembly_label_next_inst(Assembly* as, const char* label)
{
    ++as->priv_->n_next_labels;
    as->priv_->next_labels = xrealloc(as->priv_->next_labels, as->priv_->n_next_labels * sizeof(char *));
    as->priv_->next_labels[as->priv_->n_next_labels - 1] = strdup(label);
}