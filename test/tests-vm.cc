extern "C" {
#include "../lib/priv.h"
}

#include <gtest/gtest.h>


#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

__thread char last_err_msg[256] = {0};

bool abort_on_errors = false;

// ---- helper: build a double from a raw 64-bit pattern -------------------
static double double_from_bits(uint64_t bits) {
    double d;
    static_assert(sizeof d == sizeof bits, "double must be 64-bit");
    std::memcpy(&d, &bits, sizeof d);
    return d;
}

// =========================================================================
// create_value_* / value_* round trips
// =========================================================================

TEST(Value, NilType) {
    EXPECT_EQ(value_type(create_value_nil()), TYC_NIL);
}

TEST(Value, BoolRoundTrip) {
    VALUE t = create_value_bool(true);
    VALUE f = create_value_bool(false);
    EXPECT_EQ(value_type(t), TYC_BOOLEAN);
    EXPECT_EQ(value_type(f), TYC_BOOLEAN);
    EXPECT_TRUE(value_boolean(t));
    EXPECT_FALSE(value_boolean(f));
}

TEST(Value, IntegerRoundTrip) {
    int32_t cases[] = {0, 1, -1, 42, -42, 1000000, -1000000};
    for (int32_t in : cases) {
        VALUE v = create_value_integer(in);
        EXPECT_EQ(value_type(v), TYC_INTEGER) << "value " << in;
        EXPECT_EQ(value_integer(v), in) << "value " << in;
    }
}

TEST(Value, IntegerBoundaries) {
    // INT32_MIN / -1 are the patterns most likely to break if the nanbox
    // payload isn't sign-extended back to 32 bits on read.
    int32_t cases[] = {INT32_MIN, INT32_MIN + 1, -1, 0, 1, INT32_MAX - 1, INT32_MAX};
    for (int32_t in : cases) {
        VALUE v = create_value_integer(in);
        EXPECT_EQ(value_type(v), TYC_INTEGER) << "value " << in;
        EXPECT_EQ(value_integer(v), in) << "value " << in;
    }
}

TEST(Value, RealRoundTripFinite) {
    // Same double in, same double out → exact equality is the contract.
    double cases[] = {0.0, 1.0, -1.0, 3.14, -2.5, 1e308, 5e-324,
                      DBL_MAX, DBL_MIN, -DBL_MAX};
    for (double in : cases) {
        VALUE v = create_value_real(in);
        EXPECT_EQ(value_type(v), TYC_REAL) << "value " << in;
        EXPECT_EQ(value_real(v), in) << "value " << in;
    }
}

TEST(Value, RealNegativeZeroKeepsSign) {
    VALUE v = create_value_real(-0.0);
    EXPECT_EQ(value_type(v), TYC_REAL);
    EXPECT_DOUBLE_EQ(value_real(v), 0.0);
    EXPECT_TRUE(std::signbit(value_real(v)));  // -0.0 must stay negative
}

TEST(Value, RealInfinity) {
    VALUE pos = create_value_real(INFINITY);
    VALUE neg = create_value_real(-INFINITY);
    EXPECT_EQ(value_type(pos), TYC_REAL);
    EXPECT_EQ(value_type(neg), TYC_REAL);
    EXPECT_TRUE(std::isinf(value_real(pos)));
    EXPECT_TRUE(std::isinf(value_real(neg)));
    EXPECT_GT(value_real(pos), 0.0);
    EXPECT_LT(value_real(neg), 0.0);
}

// Highest-risk case for a NaN-boxed VM: a genuine floating-point NaN must
// still read back as a REAL rather than be mistaken for a boxed tag. The
// exact bit pattern is NOT expected to survive — canonicalization may rewrite
// the mantissa payload — so we assert only "is a real" and "is NaN".
TEST(Value, RealNaNCanonicalizesToReal) {
    uint64_t nan_bits[] = {
            0x7FF8000000000000ULL,  // canonical quiet NaN
            0xFFF8000000000000ULL,  // negative quiet NaN
            0x7FF8000000000001ULL,  // quiet NaN with payload
            0x7FFFFFFFFFFFFFFFULL,  // quiet NaN, all payload bits set
            0x7FF0000000000001ULL,  // signaling NaN
            0xFFF0000000000001ULL,  // negative signaling NaN
    };
    for (uint64_t bits : nan_bits) {
        double in = double_from_bits(bits);
        ASSERT_TRUE(std::isnan(in)) << "test input not a NaN, bits=" << bits;
        VALUE v = create_value_real(in);
        EXPECT_EQ(value_type(v), TYC_REAL) << "NaN bits=" << bits;
        EXPECT_TRUE(std::isnan(value_real(v))) << "NaN bits=" << bits;
    }
}

TEST(Value, FunctionIdxRoundTrip) {
    uint32_t cases[] = {0u, 1u, 1234u, 0x7FFFFFFFu, 0xFFFFFFFEu, 0xFFFFFFFFu};
    for (uint32_t in : cases) {
        VALUE v = create_value_function_idx(in);
        EXPECT_EQ(value_type(v), TYC_FUNCTION) << "idx " << in;
        EXPECT_EQ(value_function_idx(v), in) << "idx " << in;
    }
}

TEST(Value, HeapKeyRoundTrip) {
    TYC_TYPE types[] = {TYC_STRING, TYC_ARRAY, TYC_TABLE, TYC_NATIVE_FN__};
    HEAP_KEY keys[]  = {0u, 1u, 999u, 0x7FFFFFFFu, 0xFFFFFFFFu};
    for (TYC_TYPE ty : types) {
        for (HEAP_KEY k : keys) {
            VALUE v = create_value_heap_key(ty, k);
            EXPECT_EQ(value_type(v), ty) << "type " << ty << " key " << k;
            EXPECT_EQ(value_heap_key(v), k) << "type " << ty << " key " << k;
        }
    }
}

TEST(Value, NativePointerRoundTrip) {
    // Real user-space pointers fit in the nanbox's 48-bit payload.
    int local = 0;
    static int s_static = 0;
    void* heap = std::malloc(32);
    ASSERT_NE(heap, nullptr);

    void* cases[] = {nullptr, &local, &s_static, heap};
    for (void* p : cases) {
        VALUE v = create_value_native_pointer(p);
        EXPECT_EQ(value_type(v), TYC_NATIVE_PTR);
        EXPECT_EQ(value_native_pointer(v), p);
    }
    std::free(heap);
}

// One representative of every public type: value_type must classify each
// correctly with no cross-talk between the nanbox tag spaces.
TEST(Value, TypeMatrix) {
    EXPECT_EQ(value_type(create_value_nil()),                        TYC_NIL);
    EXPECT_EQ(value_type(create_value_bool(true)),                   TYC_BOOLEAN);
    EXPECT_EQ(value_type(create_value_bool(false)),                  TYC_BOOLEAN);
    EXPECT_EQ(value_type(create_value_integer(0)),                   TYC_INTEGER);
    EXPECT_EQ(value_type(create_value_integer(-1)),                  TYC_INTEGER);
    EXPECT_EQ(value_type(create_value_real(0.0)),                    TYC_REAL);
    EXPECT_EQ(value_type(create_value_function_idx(0)),              TYC_FUNCTION);
    EXPECT_EQ(value_type(create_value_native_pointer(nullptr)),      TYC_NATIVE_PTR);
    EXPECT_EQ(value_type(create_value_heap_key(TYC_STRING, 0)),      TYC_STRING);
    EXPECT_EQ(value_type(create_value_heap_key(TYC_ARRAY, 0)),       TYC_ARRAY);
    EXPECT_EQ(value_type(create_value_heap_key(TYC_TABLE, 0)),       TYC_TABLE);
    EXPECT_EQ(value_type(create_value_heap_key(TYC_NATIVE_FN__, 0)), TYC_NATIVE_FN__);
}

// =========================================================================
// value_is_false (truthiness — see the `bf` opcode: "branch if false or nil")
// =========================================================================

TEST(Value, IsFalseNilAndFalse) {
    EXPECT_TRUE(value_is_false(create_value_nil()));
    EXPECT_TRUE(value_is_false(create_value_bool(false)));
    EXPECT_FALSE(value_is_false(create_value_bool(true)));
}

// ASSUMPTION (Lua-style): numeric zero and heap references are TRUTHY.
// If Tyche treats 0 / 0.0 as falsy, flip these expectations.
TEST(Value, IsFalseEverythingElseIsTruthy) {
    EXPECT_FALSE(value_is_false(create_value_integer(0)));
    EXPECT_FALSE(value_is_false(create_value_integer(1)));
    EXPECT_FALSE(value_is_false(create_value_real(0.0)));
    EXPECT_FALSE(value_is_false(create_value_real(-0.0)));
    EXPECT_FALSE(value_is_false(create_value_function_idx(0)));
    EXPECT_FALSE(value_is_false(create_value_heap_key(TYC_STRING, 0)));
}

// =========================================================================
// type_name / type_is_collectable
// =========================================================================

TEST(TypeInfo, NameNonNullAndDistinct) {
    std::set<std::string> seen;
    for (int t = 0; t < TYC_COUNT__; ++t) {
        const char* name = type_name((TYC_TYPE)t);
        ASSERT_NE(name, nullptr) << "type tag " << t << " has a null name";
        EXPECT_GT(std::strlen(name), 0u) << "type tag " << t << " has an empty name";
        bool inserted = seen.insert(name).second;
        EXPECT_TRUE(inserted) << "duplicate type name \"" << name
                              << "\" at tag " << t;
    }
}

TEST(TypeInfo, Collectable) {
    // Heap-resident types are collectable...
    EXPECT_TRUE(type_is_collectable(TYC_STRING));
    EXPECT_TRUE(type_is_collectable(TYC_ARRAY));
    EXPECT_TRUE(type_is_collectable(TYC_TABLE));
    EXPECT_TRUE(type_is_collectable(TYC_NATIVE_FN__));
    // ...nanbox-resident types are not.
    EXPECT_FALSE(type_is_collectable(TYC_NIL));
    EXPECT_FALSE(type_is_collectable(TYC_BOOLEAN));
    EXPECT_FALSE(type_is_collectable(TYC_INTEGER));
    EXPECT_FALSE(type_is_collectable(TYC_REAL));
    EXPECT_FALSE(type_is_collectable(TYC_FUNCTION));
    EXPECT_FALSE(type_is_collectable(TYC_NATIVE_PTR));
}
