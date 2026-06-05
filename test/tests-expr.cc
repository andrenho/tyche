// tests-expr.cc — unit tests for the EXPRESSIONS layer declared in priv.h
//
// Covers: expr_init, expr_is_binary, unary_expr, binary_expr.
//
// "Expressions follow their counterparts in C" (VM.md). The C-vs-Lua
// divergences are asserted to the C side here and called out in comments:
//   * idiv truncates toward zero  (Lua // floors)
//   * mod takes the dividend sign  (Lua % takes the divisor sign)
// If your Lua reference implementation uses native Lua operators, those two
// will disagree with Tyche — verify which side is authoritative.
//
// unary_expr/binary_expr take a TycheVM* (for exceptions, string-heap access,
// and __op overload dispatch), so the fixture stands up a real VM. abort_on_
// errors stays false so error paths return TYC_ERR. Operator overloading via
// Tyche functions is an integration concern and is not exercised here.
//
// Adjust the include path below to match your tree.

extern "C" {
#include "../lib/priv.h"
}

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstring>

__thread char last_err_msg[256] = {0};
bool abort_on_errors = false;

static uint64_t vbits(VALUE v) {
    uint64_t b;
    std::memcpy(&b, &v, sizeof b);
    return b;
}

class ExprTest : public ::testing::Test {
protected:
    TycheVM* T = nullptr;
    Heap*    h = nullptr;

    void SetUp() override {
        T = tyc_new();
        ASSERT_NE(T, nullptr);
        h = tyc_heap(T);
        ASSERT_NE(h, nullptr);
        expr_init();  // build dispatch tables (idempotent)
    }
    void TearDown() override { if (T) tyc_destroy(T); }

    VALUE str(const char* s) {
        return create_value_heap_key(TYC_STRING, heap_add_string(h, s, false));
    }

    // Run an op asserting success; return the result value.
    VALUE bin(TYC_EXPR op, VALUE a, VALUE b) {
        VALUE r = create_value_nil();
        EXPECT_EQ(binary_expr(T, op, a, b, &r), TYC_OK);
        return r;
    }
    VALUE un(TYC_EXPR op, VALUE a) {
        VALUE r = create_value_nil();
        EXPECT_EQ(unary_expr(T, op, a, &r), TYC_OK);
        return r;
    }

    // Type-checked result assertions (guarded so a wrong type can't trigger a
    // bad-typed accessor).
    void expect_int(VALUE r, int32_t want) {
        ASSERT_EQ(value_type(r), TYC_INTEGER);
        EXPECT_EQ(value_integer(r), want);
    }
    void expect_real(VALUE r, double want) {
        ASSERT_EQ(value_type(r), TYC_REAL);
        EXPECT_DOUBLE_EQ(value_real(r), want);
    }
    void expect_bool(VALUE r, bool want) {
        // ASSUMPTION: comparisons yield TYC_BOOLEAN. If your VM returns
        // integer 0/1 (C-style), change these to integer checks.
        ASSERT_EQ(value_type(r), TYC_BOOLEAN);
        EXPECT_EQ(value_boolean(r), want);
    }
    double numeric(VALUE r) {  // int or real → double (for type-ambiguous ops)
        if (value_type(r) == TYC_INTEGER) return (double)value_integer(r);
        if (value_type(r) == TYC_REAL)    return value_real(r);
        ADD_FAILURE() << "expected a numeric result, got type " << value_type(r);
        return std::nan("");
    }
};

// =========================================================================
// expr_is_binary — arity classification of every op
// =========================================================================

TEST_F(ExprTest, IsBinaryClassification) {
    auto unary = [](int op) { return op == TX_NOT || op == TX_NEG || op == TX_HASH; };
    for (int op = 0; op < TX_COUNT__; ++op)
        EXPECT_EQ(expr_is_binary((TYC_EXPR)op), !unary(op)) << "op index " << op;
}

// =========================================================================
// Integer / real arithmetic
// =========================================================================

TEST_F(ExprTest, IntegerArithmeticStaysInteger) {
    expect_int(bin(TX_SUM, create_value_integer(2), create_value_integer(3)), 5);
    expect_int(bin(TX_SUB, create_value_integer(5), create_value_integer(3)), 2);
    expect_int(bin(TX_MUL, create_value_integer(4), create_value_integer(6)), 24);
}

TEST_F(ExprTest, DivisionIsAlwaysReal) {  // doc: "Float division"
    expect_real(bin(TX_DIV, create_value_integer(6), create_value_integer(2)), 3.0);
    expect_real(bin(TX_DIV, create_value_integer(7), create_value_integer(2)), 3.5);
}

TEST_F(ExprTest, IntegerDivisionTruncatesTowardZero) {
    // C semantics (NOT Lua floor //). Verify the reference matches.
    expect_int(bin(TX_IDIV, create_value_integer(7),  create_value_integer(2)),  3);
    expect_int(bin(TX_IDIV, create_value_integer(-7), create_value_integer(2)), -3);
}

TEST_F(ExprTest, ModuloFollowsDividendSign) {
    // C semantics (NOT Lua divisor-sign %). Verify the reference matches.
    expect_int(bin(TX_MOD, create_value_integer(7),  create_value_integer(3)),  1);
    expect_int(bin(TX_MOD, create_value_integer(-7), create_value_integer(3)), -1);
}

TEST_F(ExprTest, RealArithmetic) {
    expect_real(bin(TX_SUM, create_value_real(1.5), create_value_real(2.5)), 4.0);
    expect_real(bin(TX_SUB, create_value_real(5.0), create_value_real(1.5)), 3.5);
    expect_real(bin(TX_MUL, create_value_real(2.5), create_value_real(2.0)), 5.0);
}

TEST_F(ExprTest, MixedIntRealPromotesToReal) {  // ASSUMPTION: C-like promotion
    expect_real(bin(TX_SUM, create_value_integer(2), create_value_real(0.5)), 2.5);
    expect_real(bin(TX_MUL, create_value_real(2.0), create_value_integer(3)), 6.0);
}

TEST_F(ExprTest, PowerOperator) {
    // Result type (int vs real) is left to the impl; only the value is checked.
    EXPECT_DOUBLE_EQ(numeric(bin(TX_POW, create_value_integer(2), create_value_integer(10))), 1024.0);
    EXPECT_NEAR(numeric(bin(TX_POW, create_value_integer(2), create_value_real(0.5))), 1.41421356, 1e-6);
}

// =========================================================================
// Bitwise (integer operands)
// =========================================================================

TEST_F(ExprTest, BitwiseAndOrXor) {
    expect_int(bin(TX_AND, create_value_integer(0xF0), create_value_integer(0x0F)), 0x00);
    expect_int(bin(TX_AND, create_value_integer(0xFF), create_value_integer(0x0F)), 0x0F);
    expect_int(bin(TX_OR,  create_value_integer(0xF0), create_value_integer(0x0F)), 0xFF);
    expect_int(bin(TX_XOR, create_value_integer(0xFF), create_value_integer(0x0F)), 0xF0);
}

TEST_F(ExprTest, ShiftLeftRight) {
    expect_int(bin(TX_SHL, create_value_integer(1),   create_value_integer(4)), 16);
    expect_int(bin(TX_SHL, create_value_integer(3),   create_value_integer(2)), 12);
    expect_int(bin(TX_SHR, create_value_integer(256), create_value_integer(4)), 16);
}

// =========================================================================
// Comparisons  (see expect_bool's boolean-result assumption)
// =========================================================================

TEST_F(ExprTest, EqualityIntegers) {
    expect_bool(bin(TX_EQ,  create_value_integer(5), create_value_integer(5)), true);
    expect_bool(bin(TX_EQ,  create_value_integer(5), create_value_integer(6)), false);
    expect_bool(bin(TX_NEQ, create_value_integer(5), create_value_integer(6)), true);
    expect_bool(bin(TX_NEQ, create_value_integer(5), create_value_integer(5)), false);
}

TEST_F(ExprTest, OrderingIntegers) {
    expect_bool(bin(TX_LT,  create_value_integer(1), create_value_integer(2)), true);
    expect_bool(bin(TX_LT,  create_value_integer(2), create_value_integer(1)), false);
    expect_bool(bin(TX_LTE, create_value_integer(2), create_value_integer(2)), true);
    expect_bool(bin(TX_GT,  create_value_integer(3), create_value_integer(2)), true);
    expect_bool(bin(TX_GTE, create_value_integer(2), create_value_integer(2)), true);
    expect_bool(bin(TX_GTE, create_value_integer(1), create_value_integer(2)), false);
}

TEST_F(ExprTest, EqualityNilAndCrossType) {
    expect_bool(bin(TX_EQ, create_value_nil(), create_value_nil()), true);
    expect_bool(bin(TX_EQ, create_value_nil(), create_value_integer(0)), false);
    expect_bool(bin(TX_EQ, create_value_integer(1), str("1")), false);
}

TEST_F(ExprTest, EqualityStrings) {
    expect_bool(bin(TX_EQ,  str("abc"), str("abc")), true);   // interned identity
    expect_bool(bin(TX_EQ,  str("abc"), str("abd")), false);
    expect_bool(bin(TX_NEQ, str("abc"), str("abd")), true);
}

TEST_F(ExprTest, RealComparison) {
    expect_bool(bin(TX_LT, create_value_real(1.5), create_value_real(2.5)), true);
    expect_bool(bin(TX_EQ, create_value_real(2.0), create_value_real(2.0)), true);
}

TEST_F(ExprTest, MixedNumericComparison) {  // ASSUMPTION: int/real numeric eq
    expect_bool(bin(TX_EQ, create_value_integer(1), create_value_real(1.0)), true);
    expect_bool(bin(TX_LT, create_value_integer(1), create_value_real(1.5)), true);
}

// =========================================================================
// Unary: neg, not, hash
// =========================================================================

TEST_F(ExprTest, NegInteger) {
    expect_int(un(TX_NEG, create_value_integer(5)),  -5);
    expect_int(un(TX_NEG, create_value_integer(-3)),  3);
}

TEST_F(ExprTest, NegReal) {
    expect_real(un(TX_NEG, create_value_real(2.5)), -2.5);
}

TEST_F(ExprTest, NotBoolean) {  // doc: "negate boolean"
    expect_bool(un(TX_NOT, create_value_bool(true)),  false);
    expect_bool(un(TX_NOT, create_value_bool(false)), true);
}

TEST_F(ExprTest, NotIntegerIsBitwiseComplement) {  // doc: "invert value bits"
    expect_int(un(TX_NOT, create_value_integer(0)),    -1);    // ~0
    expect_int(un(TX_NOT, create_value_integer(0xFF)), ~0xFF); // -256
}

TEST_F(ExprTest, HashIsDeterministic) {
    VALUE h1 = un(TX_HASH, create_value_integer(5));
    VALUE h2 = un(TX_HASH, create_value_integer(5));
    EXPECT_EQ(vbits(h1), vbits(h2));            // same input → same hash
    VALUE hs1 = un(TX_HASH, str("abc"));
    VALUE hs2 = un(TX_HASH, str("abc"));
    EXPECT_EQ(vbits(hs1), vbits(hs2));          // interned strings hash equally
}

TEST_F(ExprTest, HashRejectsNonHashable) {
    // ASSUMPTION: reals/arrays/nils can't be keys, so they can't be hashed.
    VALUE r;
    EXPECT_EQ(unary_expr(T, TX_HASH, create_value_real(1.5), &r), TYC_ERR);
    EXPECT_EQ(unary_expr(T, TX_HASH, create_value_nil(),     &r), TYC_ERR);
    VALUE arr = create_value_heap_key(TYC_ARRAY, heap_add_array(h));
    EXPECT_EQ(unary_expr(T, TX_HASH, arr, &r), TYC_ERR);
}

// =========================================================================
// Error paths
// =========================================================================

TEST_F(ExprTest, IntegerDivByZeroErrors) {
    VALUE r;
    EXPECT_EQ(binary_expr(T, TX_IDIV, create_value_integer(5), create_value_integer(0), &r), TYC_ERR);
    EXPECT_EQ(binary_expr(T, TX_MOD,  create_value_integer(5), create_value_integer(0), &r), TYC_ERR);
}

TEST_F(ExprTest, ArithmeticOnIncompatibleTypesErrors) {  // ASSUMPTION
    VALUE r;
    EXPECT_EQ(binary_expr(T, TX_SUM, create_value_integer(1), create_value_nil(), &r), TYC_ERR);
    EXPECT_EQ(binary_expr(T, TX_SUM, create_value_nil(),      create_value_nil(), &r), TYC_ERR);
    EXPECT_EQ(binary_expr(T, TX_SUB, create_value_integer(1), str("x"),           &r), TYC_ERR);
}

TEST_F(ExprTest, BitwiseOnRealErrors) {  // ASSUMPTION: no bit ops on reals (C-like)
    VALUE r;
    EXPECT_EQ(binary_expr(T, TX_AND, create_value_real(1.0), create_value_real(2.0), &r), TYC_ERR);
    EXPECT_EQ(binary_expr(T, TX_SHL, create_value_real(1.0), create_value_integer(2), &r), TYC_ERR);
}

TEST_F(ExprTest, BinaryOnPlainTablesErrors) {  // ASSUMPTION: no __sum overload
    VALUE t1 = create_value_heap_key(TYC_TABLE, heap_add_table(h));
    VALUE t2 = create_value_heap_key(TYC_TABLE, heap_add_table(h));
    VALUE r;
    EXPECT_EQ(binary_expr(T, TX_SUM, t1, t2, &r), TYC_ERR);
}

// Open question: float division by zero — IEEE/C gives +/-inf with no trap,
// but the VM may choose to throw. Enable with whichever you implement.
TEST_F(ExprTest, DISABLED_RealDivByZeroIsInfinity) {
    VALUE r = bin(TX_DIV, create_value_real(1.0), create_value_real(0.0));
    ASSERT_EQ(value_type(r), TYC_REAL);
    EXPECT_TRUE(std::isinf(value_real(r)));
}
