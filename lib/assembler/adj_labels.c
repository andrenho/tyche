#include "assembler_priv.h"

#include "khash.h"

KHASH_MAP_INIT_STR(LBL, uint32_t)

TYC_RESULT assembler_adjust_labels(Assembly* assembly)
{
    int ret;

    for (uint32_t i = 0; i < assembly->functions_n; ++i) {
        uint32_t pc = 0;
        khash_t(LBL)* label_pc = kh_init_LBL();

        // find labels
        for (uint32_t j = 0; j < assembly->functions[i].n_instructions; ++j) {
            AssemblyInstruction* inst = &assembly->functions[i].instructions[j];
            for (uint32_t k = 0; k < inst->n_labels; ++k) {
                khiter_t kk = kh_put_LBL(label_pc, inst->labels[k], &ret);
                if (ret)
                    kh_value(label_pc, kk) = pc;
            }
            switch (inst->operator.type) {
                case OP_NONE:
                    abort();
                case OP_INT:
                    pc += instruction_size(inst->instruction, inst->operator.v.i);
                    break;
                case OP_LABEL:
                    pc += instruction_size(inst->instruction, 0);
            }
        }

        // replace labels
        for (uint32_t j = 0; j < assembly->functions[i].n_instructions; ++j) {
            AssemblyInstruction* inst = &assembly->functions[i].instructions[j];
            if (inst->operator.type == OP_LABEL) {
                khiter_t k = kh_get_LBL(label_pc, inst->operator.v.label);
                if (k == kh_end(label_pc))
                    ERROR("Label '%s' not found", inst->operator.v.label);
                inst->operator.type = OP_INT;
                inst->operator.v.i = kh_value(label_pc, k);
            }
        }

        kh_destroy_LBL(label_pc);
    }

    return T_OK;
}