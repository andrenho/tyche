#include "priv.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define TRYX(x) if (((rr) = (x)) != TYC_OK) { return (rr); }

#define EPSILON 0.000000000001

static bool was_init = false;

typedef TYC_RESULT(*EXPR_FN)(TycheVM* T, VALUE a, VALUE b, VALUE* r);
static EXPR_FN expr_fn[TX_COUNT__][TYC_COUNT__][TYC_COUNT__];

static TYC_RESULT default_binary_op(TycheVM* T, VALUE a, VALUE b, VALUE* r) {
    (void) r;
    char buf[500];
    snprintf(buf, sizeof buf, "Incorrect types in expression: %s and %s", type_name(value_type(a)), type_name(value_type(b)));
    tyc_throw(T, buf);
    return TYC_ERR;
}

static TYC_RESULT default_unary_op(TycheVM* T, VALUE a, VALUE b, VALUE* r) {
    (void) r; (void) b;
    char buf[500];
    snprintf(buf, sizeof buf, "Incorrect type in expression: %s", type_name(value_type(a)));
    tyc_throw(T, buf);
    return TYC_ERR;
}

#define OP(name) static TYC_RESULT name(TycheVM* T, VALUE a, VALUE b, VALUE* r)
OP(and_bool)     { (void) T; *r = create_value_bool(value_boolean(a) && value_boolean(b)); return TYC_OK; }
OP(or_bool)      { (void) T; *r = create_value_bool(value_boolean(a) || value_boolean(b)); return TYC_OK; }
OP(xor_bool)     { (void) T; *r = create_value_bool(value_boolean(a) ^ value_boolean(b)); return TYC_OK; }
OP(eq_bool)      { (void) T; *r = create_value_bool(value_boolean(a) == value_boolean(b)); return TYC_OK; }
OP(neq_bool)     { (void) T; *r = create_value_bool(value_boolean(a) != value_boolean(b)); return TYC_OK; }
OP(not_bool)     { (void) T, (void) b; *r = create_value_bool(!value_boolean(a)); return TYC_OK; }

OP(sum_int_int)  { (void) T; *r = create_value_integer(value_integer(a) + value_integer(b)); return TYC_OK; }
OP(sub_int_int)  { (void) T; *r = create_value_integer(value_integer(a) - value_integer(b)); return TYC_OK; }
OP(mul_int_int)  { (void) T; *r = create_value_integer(value_integer(a) * value_integer(b)); return TYC_OK; }
OP(div_int_int)  { (void) T; *r = create_value_real((TYCHE_REAL) value_integer(a) / (TYCHE_REAL) value_integer(b)); return TYC_OK; }
OP(idiv_int_int) { (void) T; *r = create_value_integer(value_integer(a) / value_integer(b)); return TYC_OK; }
OP(eq_int_int)   { (void) T; *r = create_value_bool(value_integer(a)==value_integer(b)); return TYC_OK; }
OP(neq_int_int)  { (void) T; *r = create_value_bool(value_integer(a)!=value_integer(b)); return TYC_OK; }
OP(lt_int_int)   { (void) T; *r = create_value_bool(value_integer(a)<value_integer(b)); return TYC_OK; }
OP(lte_int_int)  { (void) T; *r = create_value_bool(value_integer(a)<=value_integer(b)); return TYC_OK; }
OP(gt_int_int)   { (void) T; *r = create_value_bool(value_integer(a)>value_integer(b)); return TYC_OK; }
OP(gte_int_int)  { (void) T; *r = create_value_bool(value_integer(a)>=value_integer(b)); return TYC_OK; }
OP(and_int_int)  { (void) T; *r = create_value_integer(value_integer(a) & value_integer(b)); return TYC_OK; }
OP(or_int_int)   { (void) T; *r = create_value_integer(value_integer(a) | value_integer(b)); return TYC_OK; }
OP(xor_int_int)  { (void) T; *r = create_value_integer(value_integer(a) ^ value_integer(b)); return TYC_OK; }
OP(pow_int_int)  { (void) T; *r = create_value_integer((int32_t) powl(value_integer(a), value_integer(b))); return TYC_OK; }
OP(shl_int_int)  { (void) T; *r = create_value_integer(value_integer(a) << value_integer(b)); return TYC_OK; }
OP(shr_int_int)  { (void) T; *r = create_value_integer(value_integer(a) >> value_integer(b)); return TYC_OK; }
OP(mod_int_int)  { (void) T; *r = create_value_integer(value_integer(a) % value_integer(b)); return TYC_OK; }
OP(not_int)      { (void) T, (void) b; *r = create_value_integer(~value_integer(a)); return TYC_OK; }
OP(neg_int)      { (void) T, (void) b; *r = create_value_integer(-value_integer(a)); return TYC_OK; }

OP(sum_real)  { (void) T; *r = create_value_real(value_real(a) + value_real(b)); return TYC_OK; }
OP(sub_real)  { (void) T; *r = create_value_real(value_real(a) - value_real(b)); return TYC_OK; }
OP(mul_real)  { (void) T; *r = create_value_real(value_real(a) * value_real(b)); return TYC_OK; }
OP(div_real)  { (void) T; *r = create_value_real(value_real(a) / value_real(b)); return TYC_OK; }
OP(idiv_real) { (void) T; *r = create_value_integer((int32_t) (value_real(a) / value_real(b))); return TYC_OK; }
OP(eq_real)   { (void) T; *r = create_value_bool(fabs(value_real(a) - value_real(b)) <= EPSILON); return TYC_OK; }
OP(neq_real)  { (void) T; *r = create_value_bool(fabs(value_real(a) - value_real(b)) > EPSILON); return TYC_OK; }
OP(lt_real)   { (void) T; *r = create_value_bool(value_real(a) < value_real(b)); return TYC_OK; }
OP(lte_real)  { (void) T; *r = create_value_bool(value_real(a) <= value_real(b)); return TYC_OK; }
OP(gt_real)   { (void) T; *r = create_value_bool(value_real(a) > value_real(b)); return TYC_OK; }
OP(gte_real)  { (void) T; *r = create_value_bool(value_real(a) >= value_real(b)); return TYC_OK; }
OP(pow_real)  { (void) T; *r = create_value_real(pow(value_real(a), value_real(b))); return TYC_OK; }
OP(neg_real)  { (void) T, (void) b; *r = create_value_real(-value_real(a)); return TYC_OK; }

OP(sum_str_str) {
    TYC_RESULT rr;
    const char *s1, *s2;
    TRYX(heap_get_string(tyc_heap(T), value_heap_key(a), &s1))
    TRYX(heap_get_string(tyc_heap(T), value_heap_key(b), &s2))
    char* str = xcalloc(1, strlen(s1) + strlen(s2) + 1);
    strcat(str, s1);
    strcpy(&str[strlen(s1)], s2);
    *r = create_value_heap_key(TYC_STRING, heap_add_string(tyc_heap(T), str, false));
    free(str);
    return TYC_OK;
}

static void setup_real_expr(TYC_TYPE type_a, TYC_TYPE type_b)
{
    expr_fn[TX_SUM][type_a][type_b] = sum_real;
    expr_fn[TX_SUB][type_a][type_b] = sub_real;
    expr_fn[TX_MUL][type_a][type_b] = mul_real;
    expr_fn[TX_DIV][type_a][type_b] = div_real;
    expr_fn[TX_IDIV][type_a][type_b] = idiv_real;
    expr_fn[TX_EQ][type_a][type_b] = eq_real;
    expr_fn[TX_NEQ][type_a][type_b] = neq_real;
    expr_fn[TX_LT][type_a][type_b] = lt_real;
    expr_fn[TX_LTE][type_a][type_b] = lte_real;
    expr_fn[TX_GT][type_a][type_b] = gt_real;
    expr_fn[TX_GTE][type_a][type_b] = gte_real;
    expr_fn[TX_POW][type_a][type_b] = pow_real;
}

void expr_init(void)
{
    if (was_init)
        return;

    // default (reject)
    for (TYC_EXPR i = 0; i < TX_COUNT__; ++i)
        for (TYC_TYPE j = 0; j < TYC_COUNT__; ++j)
            for (TYC_TYPE k = 0; k < TYC_COUNT__; ++k)
                expr_fn[i][j][k] = expr_is_binary(i) ? default_binary_op : default_unary_op;

    // boolean
    expr_fn[TX_AND][TYC_BOOLEAN][TYC_BOOLEAN] = and_bool;
    expr_fn[TX_OR][TYC_BOOLEAN][TYC_BOOLEAN] = or_bool;
    expr_fn[TX_XOR][TYC_BOOLEAN][TYC_BOOLEAN] = xor_bool;
    expr_fn[TX_EQ][TYC_BOOLEAN][TYC_BOOLEAN] = eq_bool;
    expr_fn[TX_NEQ][TYC_BOOLEAN][TYC_BOOLEAN] = neq_bool;
    expr_fn[TX_NOT][TYC_BOOLEAN][TYC_NIL] = not_bool;

    // integer
    expr_fn[TX_SUM][TYC_INTEGER][TYC_INTEGER] = sum_int_int;
    expr_fn[TX_SUB][TYC_INTEGER][TYC_INTEGER] = sub_int_int;
    expr_fn[TX_MUL][TYC_INTEGER][TYC_INTEGER] = mul_int_int;
    expr_fn[TX_DIV][TYC_INTEGER][TYC_INTEGER] = div_int_int;
    expr_fn[TX_IDIV][TYC_INTEGER][TYC_INTEGER] = idiv_int_int;
    expr_fn[TX_EQ][TYC_INTEGER][TYC_INTEGER] = eq_int_int;
    expr_fn[TX_NEQ][TYC_INTEGER][TYC_INTEGER] = neq_int_int;
    expr_fn[TX_LT][TYC_INTEGER][TYC_INTEGER] = lt_int_int;
    expr_fn[TX_LTE][TYC_INTEGER][TYC_INTEGER] = lte_int_int;
    expr_fn[TX_GT][TYC_INTEGER][TYC_INTEGER] = gt_int_int;
    expr_fn[TX_GTE][TYC_INTEGER][TYC_INTEGER] = gte_int_int;
    expr_fn[TX_AND][TYC_INTEGER][TYC_INTEGER] = and_int_int;
    expr_fn[TX_OR][TYC_INTEGER][TYC_INTEGER] = or_int_int;
    expr_fn[TX_XOR][TYC_INTEGER][TYC_INTEGER] = xor_int_int;
    expr_fn[TX_POW][TYC_INTEGER][TYC_INTEGER] = pow_int_int;
    expr_fn[TX_SHL][TYC_INTEGER][TYC_INTEGER] = shl_int_int;
    expr_fn[TX_SHR][TYC_INTEGER][TYC_INTEGER] = shr_int_int;
    expr_fn[TX_MOD][TYC_INTEGER][TYC_INTEGER] = mod_int_int;
    expr_fn[TX_NOT][TYC_INTEGER][TYC_NIL] = not_int;
    expr_fn[TX_NEG][TYC_INTEGER][TYC_NIL] = neg_int;

    // real
    setup_real_expr(TYC_INTEGER, TYC_REAL);
    setup_real_expr(TYC_REAL, TYC_INTEGER);
    setup_real_expr(TYC_REAL, TYC_REAL);
    expr_fn[TX_NEG][TYC_REAL][TYC_NIL] = neg_real;

    // strings
    expr_fn[TX_SUM][TYC_STRING][TYC_STRING] = sum_str_str;

    was_init = true;
}

TYC_RESULT unary_expr(TycheVM* T, TYC_EXPR op, VALUE a, VALUE* result)
{
    return expr_fn[op][value_type(a)][TYC_NIL](T, a, create_value_nil(), result);
}

TYC_RESULT binary_expr(TycheVM* T, TYC_EXPR op, VALUE a, VALUE b, VALUE* result)
{
    return expr_fn[op][value_type(a)][value_type(b)](T, a, b, result);
}

bool expr_is_binary(TYC_EXPR op)
{
    return op != TX_NOT && op != TX_NEG;
}
