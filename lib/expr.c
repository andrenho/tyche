#include "priv.h"

#include <stdbool.h>

static bool was_init = false;

typedef TYC_RESULT(*BIN_EXPR_FN)(VALUE, VALUE, VALUE*);
static BIN_EXPR_FN bin_expr_fn[TX_COUNT__][TT_COUNT__][TT_COUNT__];

static TYC_RESULT default_bin_op(VALUE a, VALUE b, VALUE* r) { (void) a; (void) b, (void) r; return T_ERR_EXPR_INCORRECT_TYPES; }

#define BIN_OP(name) static TYC_RESULT name(VALUE a, VALUE b, VALUE* r)
BIN_OP(sum_int) { *r = create_value_integer(value_integer(a) + value_integer(b)); return T_OK; }
BIN_OP(sub_int) { *r = create_value_integer(value_integer(a) - value_integer(b)); return T_OK; }

void expr_init(void)
{
    if (was_init)
        return;

    for (size_t i = 0; i < TX_COUNT__; ++i)
        for (size_t j = 0; j < TT_COUNT__; ++j)
            for (size_t k = 0; k < TT_COUNT__; ++k)
                bin_expr_fn[i][j][k] = default_bin_op;

    bin_expr_fn[TX_SUM][TT_INTEGER][TT_INTEGER] = sum_int;
    bin_expr_fn[TX_SUBTRACT][TT_INTEGER][TT_INTEGER] = sub_int;

    was_init = true;
}

TYC_RESULT binary_expr(TYC_EXPR op, VALUE a, VALUE b, VALUE* result)
{
    return bin_expr_fn[op][value_type(a)][value_type(b)](a, b, result);
}