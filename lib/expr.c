#include "priv.h"

#include <stdbool.h>
#include <math.h>

static bool was_init = false;

typedef TYC_RESULT(*BIN_EXPR_FN)(VALUE, VALUE, VALUE*);
static BIN_EXPR_FN bin_expr_fn[TX_COUNT__][TT_COUNT__][TT_COUNT__];

static TYC_RESULT default_bin_op(VALUE a, VALUE b, VALUE* r) { (void) a; (void) b, (void) r; return T_ERR_EXPR_INCORRECT_TYPES; }

#define BIN_OP(name) static TYC_RESULT name(VALUE a, VALUE b, VALUE* r)
BIN_OP(sum_int_int)  { *r = create_value_integer(value_integer(a) + value_integer(b)); return T_OK; }
BIN_OP(sub_int_int)  { *r = create_value_integer(value_integer(a) - value_integer(b)); return T_OK; }
BIN_OP(mul_int_int)  { *r = create_value_integer(value_integer(a) * value_integer(b)); return T_OK; }
BIN_OP(idiv_int_int) { *r = create_value_integer(value_integer(a) / value_integer(b)); return T_OK; }
BIN_OP(eq_int_int)   { *r = create_value_from_bool(value_integer(a) == value_integer(b)); return T_OK; }
BIN_OP(neq_int_int)  { *r = create_value_from_bool(value_integer(a) != value_integer(b)); return T_OK; }
BIN_OP(lt_int_int)   { *r = create_value_from_bool(value_integer(a) < value_integer(b)); return T_OK; }
BIN_OP(lte_int_int)  { *r = create_value_from_bool(value_integer(a) <= value_integer(b)); return T_OK; }
BIN_OP(gt_int_int)   { *r = create_value_from_bool(value_integer(a) > value_integer(b)); return T_OK; }
BIN_OP(gte_int_int)  { *r = create_value_from_bool(value_integer(a) >= value_integer(b)); return T_OK; }
BIN_OP(and_int_int)  { *r = create_value_integer(value_integer(a) & value_integer(b)); return T_OK; }
BIN_OP(or_int_int)   { *r = create_value_integer(value_integer(a) | value_integer(b)); return T_OK; }
BIN_OP(xor_int_int)  { *r = create_value_integer(value_integer(a) ^ value_integer(b)); return T_OK; }
BIN_OP(pow_int_int)  { *r = create_value_integer(pow(value_integer(a), value_integer(b))); return T_OK; }
BIN_OP(shl_int_int)  { *r = create_value_integer(value_integer(a) << value_integer(b)); return T_OK; }
BIN_OP(shr_int_int)  { *r = create_value_integer(value_integer(a) >> value_integer(b)); return T_OK; }
BIN_OP(mod_int_int)  { *r = create_value_integer(value_integer(a) % value_integer(b)); return T_OK; }

void expr_init(void)
{
    if (was_init)
        return;

    for (size_t i = 0; i < TX_COUNT__; ++i)
        for (size_t j = 0; j < TT_COUNT__; ++j)
            for (size_t k = 0; k < TT_COUNT__; ++k)
                bin_expr_fn[i][j][k] = default_bin_op;

    bin_expr_fn[TX_SUM][TT_INTEGER][TT_INTEGER] = sum_int_int;
    bin_expr_fn[TX_SUB][TT_INTEGER][TT_INTEGER] = sub_int_int;
    bin_expr_fn[TX_MUL][TT_INTEGER][TT_INTEGER] = mul_int_int;
    bin_expr_fn[TX_IDIV][TT_INTEGER][TT_INTEGER] = idiv_int_int;
    bin_expr_fn[TX_EQ][TT_INTEGER][TT_INTEGER] = eq_int_int;
    bin_expr_fn[TX_NEQ][TT_INTEGER][TT_INTEGER] = neq_int_int;
    bin_expr_fn[TX_LT][TT_INTEGER][TT_INTEGER] = lt_int_int;
    bin_expr_fn[TX_LTE][TT_INTEGER][TT_INTEGER] = lte_int_int;
    bin_expr_fn[TX_GT][TT_INTEGER][TT_INTEGER] = gt_int_int;
    bin_expr_fn[TX_GTE][TT_INTEGER][TT_INTEGER] = gte_int_int;
    bin_expr_fn[TX_AND][TT_INTEGER][TT_INTEGER] = and_int_int;
    bin_expr_fn[TX_OR][TT_INTEGER][TT_INTEGER] = or_int_int;
    bin_expr_fn[TX_XOR][TT_INTEGER][TT_INTEGER] = xor_int_int;
    bin_expr_fn[TX_POW][TT_INTEGER][TT_INTEGER] = pow_int_int;
    bin_expr_fn[TX_SHL][TT_INTEGER][TT_INTEGER] = shl_int_int;
    bin_expr_fn[TX_SHR][TT_INTEGER][TT_INTEGER] = shr_int_int;
    bin_expr_fn[TX_MOD][TT_INTEGER][TT_INTEGER] = mod_int_int;

    was_init = true;
}

TYC_RESULT binary_expr(TYC_EXPR op, VALUE a, VALUE b, VALUE* result)
{
    return bin_expr_fn[op][value_type(a)][value_type(b)](a, b, result);
}