#include "compiler_priv.h"

#include <stdlib.h>

Assembly* assembly_new()
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
    // TODO
}

void assembly_add_const_real(Assembly* as, uint32_t c_id, double value)
{
    // TODO
}

void assembly_add_function(Assembly* as, uint32_t f_id)
{
    // TODO
}
