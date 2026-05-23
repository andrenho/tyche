#include "priv.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define TRYX(x) if (((rr) = (x)) != T_OK) { return (rr); }

#define EPSILON 0.000000000001

static bool was_init = false;

typedef TYC_RESULT(*BIN_EXPR_FN)(TycheVM*, VALUE, VALUE, VALUE*);
static BIN_EXPR_FN bin_expr_fn[TX_COUNT__][TT_COUNT__][TT_COUNT__];

static TYC_RESULT default_bin_op(TycheVM* T, VALUE a, VALUE b, VALUE* r) {
    (void) T; (void) a; (void) b, (void) r;
    ERROR("Incorrect types in expression: %s and %s", type_name(value_type(a)), type_name(value_type(b)))
}

#define BIN_OP(name) static TYC_RESULT name(TycheVM* T, VALUE a, VALUE b, VALUE* r)
BIN_OP(sum_int_int)  { (void) T; *r = create_value_integer(value_integer(a) + value_integer(b)); return T_OK; }
BIN_OP(sub_int_int)  { (void) T; *r = create_value_integer(value_integer(a) - value_integer(b)); return T_OK; }
BIN_OP(mul_int_int)  { (void) T; *r = create_value_integer(value_integer(a) * value_integer(b)); return T_OK; }
BIN_OP(div_int_int)  { (void) T; *r = create_value_real((T_REAL) value_integer(a) / (T_REAL) value_integer(b)); return T_OK; }
BIN_OP(idiv_int_int) { (void) T; *r = create_value_integer(value_integer(a) / value_integer(b)); return T_OK; }
BIN_OP(eq_int_int)   { (void) T; *r = create_value_bool(value_integer(a)==value_integer(b)); return T_OK; }
BIN_OP(neq_int_int)  { (void) T; *r = create_value_bool(value_integer(a)!=value_integer(b)); return T_OK; }
BIN_OP(lt_int_int)   { (void) T; *r = create_value_bool(value_integer(a)<value_integer(b)); return T_OK; }
BIN_OP(lte_int_int)  { (void) T; *r = create_value_bool(value_integer(a)<=value_integer(b)); return T_OK; }
BIN_OP(gt_int_int)   { (void) T; *r = create_value_bool(value_integer(a)>value_integer(b)); return T_OK; }
BIN_OP(gte_int_int)  { (void) T; *r = create_value_bool(value_integer(a)>=value_integer(b)); return T_OK; }
BIN_OP(and_int_int)  { (void) T; *r = create_value_integer(value_integer(a) & value_integer(b)); return T_OK; }
BIN_OP(or_int_int)   { (void) T; *r = create_value_integer(value_integer(a) | value_integer(b)); return T_OK; }
BIN_OP(xor_int_int)  { (void) T; *r = create_value_integer(value_integer(a) ^ value_integer(b)); return T_OK; }
BIN_OP(pow_int_int)  { (void) T; *r = create_value_integer((int32_t) powl(value_integer(a), value_integer(b))); return T_OK; }
BIN_OP(shl_int_int)  { (void) T; *r = create_value_integer(value_integer(a) << value_integer(b)); return T_OK; }
BIN_OP(shr_int_int)  { (void) T; *r = create_value_integer(value_integer(a) >> value_integer(b)); return T_OK; }
BIN_OP(mod_int_int)  { (void) T; *r = create_value_integer(value_integer(a) % value_integer(b)); return T_OK; }

BIN_OP(sum_real)  { (void) T; *r = create_value_real(value_real(a) + value_real(b)); return T_OK; }
BIN_OP(sub_real)  { (void) T; *r = create_value_real(value_real(a) - value_real(b)); return T_OK; }
BIN_OP(mul_real)  { (void) T; *r = create_value_real(value_real(a) * value_real(b)); return T_OK; }
BIN_OP(div_real)  { (void) T; *r = create_value_real(value_real(a) / value_real(b)); return T_OK; }
BIN_OP(idiv_real) { (void) T; *r = create_value_integer((int32_t) (value_real(a) / value_real(b))); return T_OK; }
BIN_OP(eq_real)   { (void) T; *r = create_value_bool(fabs(value_real(a) - value_real(b)) <= EPSILON); return T_OK; }
BIN_OP(neq_real)  { (void) T; *r = create_value_bool(fabs(value_real(a) - value_real(b)) > EPSILON); return T_OK; }
BIN_OP(lt_real)   { (void) T; *r = create_value_bool(value_real(a) < value_real(b)); return T_OK; }
BIN_OP(lte_real)  { (void) T; *r = create_value_bool(value_real(a) <= value_real(b)); return T_OK; }
BIN_OP(gt_real)   { (void) T; *r = create_value_bool(value_real(a) > value_real(b)); return T_OK; }
BIN_OP(gte_real)  { (void) T; *r = create_value_bool(value_real(a) >= value_real(b)); return T_OK; }
BIN_OP(pow_real)  { (void) T; *r = create_value_real(pow(value_real(a), value_real(b))); return T_OK; }

BIN_OP(sum_str_str) {
    TYC_RESULT rr;
    const char *s1, *s2;
    TRYX(heap_get_string(tyc_heap(T), value_heap_key(a), &s1))
    TRYX(heap_get_string(tyc_heap(T), value_heap_key(b), &s2))
    char* str = xcalloc(1, strlen(s1) + strlen(s2) + 1);
    strcat(str, s1);
    strcpy(&str[strlen(s1)], s2);
    *r = create_value_heap_key(TT_STRING, heap_add_string(tyc_heap(T), str, false));
    free(str);
    return T_OK;
}

static void setup_real_expr(TYC_TYPE type_a, TYC_TYPE type_b)
{
    bin_expr_fn[TX_SUM][type_a][type_b] = sum_real;
    bin_expr_fn[TX_SUB][type_a][type_b] = sub_real;
    bin_expr_fn[TX_MUL][type_a][type_b] = mul_real;
    bin_expr_fn[TX_DIV][type_a][type_b] = div_real;
    bin_expr_fn[TX_IDIV][type_a][type_b] = idiv_real;
    bin_expr_fn[TX_EQ][type_a][type_b] = eq_real;
    bin_expr_fn[TX_NEQ][type_a][type_b] = neq_real;
    bin_expr_fn[TX_LT][type_a][type_b] = lt_real;
    bin_expr_fn[TX_LTE][type_a][type_b] = lte_real;
    bin_expr_fn[TX_GT][type_a][type_b] = gt_real;
    bin_expr_fn[TX_GTE][type_a][type_b] = gte_real;
    bin_expr_fn[TX_POW][type_a][type_b] = pow_real;
}

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
    bin_expr_fn[TX_DIV][TT_INTEGER][TT_INTEGER] = div_int_int;
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

    setup_real_expr(TT_INTEGER, TT_REAL);
    setup_real_expr(TT_REAL, TT_INTEGER);
    setup_real_expr(TT_REAL, TT_REAL);

    bin_expr_fn[TX_SUM][TT_STRING][TT_STRING] = sum_str_str;

    was_init = true;
}

TYC_RESULT binary_expr(TycheVM* T, TYC_EXPR op, VALUE a, VALUE b, VALUE* result)
{
    return bin_expr_fn[op][value_type(a)][value_type(b)](T, a, b, result);
}