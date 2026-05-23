#include "priv.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define TRYX(x) if (((rr) = (x)) != T_OK) { return (rr); }

#define EPSILON 0.000000000001

static bool was_init = false;

typedef TYC_RESULT(*EXPR_FN)(TycheVM* T, VALUE a, VALUE b, VALUE* r);
static EXPR_FN expr_fn[TX_COUNT__][TT_COUNT__][TT_COUNT__];

static TYC_RESULT default_op(TycheVM* T, VALUE a, VALUE b, VALUE* r) {
    (void) T; (void) a; (void) b, (void) r;
    ERROR("Incorrect types in expression: %s and %s", type_name(value_type(a)), type_name(value_type(b)))
}

#define OP(name) static TYC_RESULT name(TycheVM* T, VALUE a, VALUE b, VALUE* r)
OP(and_bool)     { (void) T; *r = create_value_bool(value_boolean(a) & value_boolean(b)); return T_OK; }
OP(or_bool)      { (void) T; *r = create_value_bool(value_boolean(a) | value_boolean(b)); return T_OK; }
OP(xor_bool)     { (void) T; *r = create_value_bool(value_boolean(a) ^ value_boolean(b)); return T_OK; }
OP(eq_bool)      { (void) T; *r = create_value_bool(value_boolean(a) == value_boolean(b)); return T_OK; }
OP(neq_bool)     { (void) T; *r = create_value_bool(value_boolean(a) != value_boolean(b)); return T_OK; }
OP(not_bool)     { (void) T, (void) b; *r = create_value_bool(!value_boolean(a)); return T_OK; }

OP(sum_int_int)  { (void) T; *r = create_value_integer(value_integer(a) + value_integer(b)); return T_OK; }
OP(sub_int_int)  { (void) T; *r = create_value_integer(value_integer(a) - value_integer(b)); return T_OK; }
OP(mul_int_int)  { (void) T; *r = create_value_integer(value_integer(a) * value_integer(b)); return T_OK; }
OP(div_int_int)  { (void) T; *r = create_value_real((T_REAL) value_integer(a) / (T_REAL) value_integer(b)); return T_OK; }
OP(idiv_int_int) { (void) T; *r = create_value_integer(value_integer(a) / value_integer(b)); return T_OK; }
OP(eq_int_int)   { (void) T; *r = create_value_bool(value_integer(a)==value_integer(b)); return T_OK; }
OP(neq_int_int)  { (void) T; *r = create_value_bool(value_integer(a)!=value_integer(b)); return T_OK; }
OP(lt_int_int)   { (void) T; *r = create_value_bool(value_integer(a)<value_integer(b)); return T_OK; }
OP(lte_int_int)  { (void) T; *r = create_value_bool(value_integer(a)<=value_integer(b)); return T_OK; }
OP(gt_int_int)   { (void) T; *r = create_value_bool(value_integer(a)>value_integer(b)); return T_OK; }
OP(gte_int_int)  { (void) T; *r = create_value_bool(value_integer(a)>=value_integer(b)); return T_OK; }
OP(and_int_int)  { (void) T; *r = create_value_integer(value_integer(a) & value_integer(b)); return T_OK; }
OP(or_int_int)   { (void) T; *r = create_value_integer(value_integer(a) | value_integer(b)); return T_OK; }
OP(xor_int_int)  { (void) T; *r = create_value_integer(value_integer(a) ^ value_integer(b)); return T_OK; }
OP(pow_int_int)  { (void) T; *r = create_value_integer((int32_t) powl(value_integer(a), value_integer(b))); return T_OK; }
OP(shl_int_int)  { (void) T; *r = create_value_integer(value_integer(a) << value_integer(b)); return T_OK; }
OP(shr_int_int)  { (void) T; *r = create_value_integer(value_integer(a) >> value_integer(b)); return T_OK; }
OP(mod_int_int)  { (void) T; *r = create_value_integer(value_integer(a) % value_integer(b)); return T_OK; }
OP(not_int)      { (void) T, (void) b; *r = create_value_integer(~value_integer(a)); return T_OK; }
OP(neg_int)      { (void) T, (void) b; *r = create_value_integer(-value_integer(a)); return T_OK; }

OP(sum_real)  { (void) T; *r = create_value_real(value_real(a) + value_real(b)); return T_OK; }
OP(sub_real)  { (void) T; *r = create_value_real(value_real(a) - value_real(b)); return T_OK; }
OP(mul_real)  { (void) T; *r = create_value_real(value_real(a) * value_real(b)); return T_OK; }
OP(div_real)  { (void) T; *r = create_value_real(value_real(a) / value_real(b)); return T_OK; }
OP(idiv_real) { (void) T; *r = create_value_integer((int32_t) (value_real(a) / value_real(b))); return T_OK; }
OP(eq_real)   { (void) T; *r = create_value_bool(fabs(value_real(a) - value_real(b)) <= EPSILON); return T_OK; }
OP(neq_real)  { (void) T; *r = create_value_bool(fabs(value_real(a) - value_real(b)) > EPSILON); return T_OK; }
OP(lt_real)   { (void) T; *r = create_value_bool(value_real(a) < value_real(b)); return T_OK; }
OP(lte_real)  { (void) T; *r = create_value_bool(value_real(a) <= value_real(b)); return T_OK; }
OP(gt_real)   { (void) T; *r = create_value_bool(value_real(a) > value_real(b)); return T_OK; }
OP(gte_real)  { (void) T; *r = create_value_bool(value_real(a) >= value_real(b)); return T_OK; }
OP(pow_real)  { (void) T; *r = create_value_real(pow(value_real(a), value_real(b))); return T_OK; }
OP(neg_real)  { (void) T, (void) b; *r = create_value_real(-value_real(a)); return T_OK; }

OP(sum_str_str) {
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
    for (size_t i = 0; i < TX_COUNT__; ++i)
        for (size_t j = 0; j < TT_COUNT__; ++j)
            for (size_t k = 0; k < TT_COUNT__; ++k)
                expr_fn[i][j][k] = default_op;

    // boolean
    expr_fn[TX_AND][TT_BOOLEAN][TT_BOOLEAN] = and_bool;
    expr_fn[TX_OR][TT_BOOLEAN][TT_BOOLEAN] = or_bool;
    expr_fn[TX_XOR][TT_BOOLEAN][TT_BOOLEAN] = xor_bool;
    expr_fn[TX_EQ][TT_BOOLEAN][TT_BOOLEAN] = eq_bool;
    expr_fn[TX_NEQ][TT_BOOLEAN][TT_BOOLEAN] = neq_bool;
    expr_fn[TX_NOT][TT_BOOLEAN][TT_NIL] = not_bool;

    // integer
    expr_fn[TX_SUM][TT_INTEGER][TT_INTEGER] = sum_int_int;
    expr_fn[TX_SUB][TT_INTEGER][TT_INTEGER] = sub_int_int;
    expr_fn[TX_MUL][TT_INTEGER][TT_INTEGER] = mul_int_int;
    expr_fn[TX_DIV][TT_INTEGER][TT_INTEGER] = div_int_int;
    expr_fn[TX_IDIV][TT_INTEGER][TT_INTEGER] = idiv_int_int;
    expr_fn[TX_EQ][TT_INTEGER][TT_INTEGER] = eq_int_int;
    expr_fn[TX_NEQ][TT_INTEGER][TT_INTEGER] = neq_int_int;
    expr_fn[TX_LT][TT_INTEGER][TT_INTEGER] = lt_int_int;
    expr_fn[TX_LTE][TT_INTEGER][TT_INTEGER] = lte_int_int;
    expr_fn[TX_GT][TT_INTEGER][TT_INTEGER] = gt_int_int;
    expr_fn[TX_GTE][TT_INTEGER][TT_INTEGER] = gte_int_int;
    expr_fn[TX_AND][TT_INTEGER][TT_INTEGER] = and_int_int;
    expr_fn[TX_OR][TT_INTEGER][TT_INTEGER] = or_int_int;
    expr_fn[TX_XOR][TT_INTEGER][TT_INTEGER] = xor_int_int;
    expr_fn[TX_POW][TT_INTEGER][TT_INTEGER] = pow_int_int;
    expr_fn[TX_SHL][TT_INTEGER][TT_INTEGER] = shl_int_int;
    expr_fn[TX_SHR][TT_INTEGER][TT_INTEGER] = shr_int_int;
    expr_fn[TX_MOD][TT_INTEGER][TT_INTEGER] = mod_int_int;
    expr_fn[TX_NOT][TT_INTEGER][TT_NIL] = not_int;
    expr_fn[TX_NEG][TT_INTEGER][TT_NIL] = neg_int;

    // real
    setup_real_expr(TT_INTEGER, TT_REAL);
    setup_real_expr(TT_REAL, TT_INTEGER);
    setup_real_expr(TT_REAL, TT_REAL);
    expr_fn[TX_NEG][TT_REAL][TT_NIL] = neg_real;

    // strings
    expr_fn[TX_SUM][TT_STRING][TT_STRING] = sum_str_str;

    was_init = true;
}

TYC_RESULT unary_expr(TycheVM* T, TYC_EXPR op, VALUE a, VALUE* result)
{
    return expr_fn[op][value_type(a)][TT_NIL](T, a, create_value_nil(), result);
}

TYC_RESULT binary_expr(TycheVM* T, TYC_EXPR op, VALUE a, VALUE b, VALUE* result)
{
    return expr_fn[op][value_type(a)][value_type(b)](T, a, b, result);
}

bool expr_is_binary(TYC_EXPR op)
{
    return op != TX_NOT && op != TX_NEG && op != TX_LEN;
}
