// tests-array.cc — unit tests for the HEAP ARRAY layer declared in priv.h
//
// Covers: array_new/destroy, array_len, array_get, array_set, array_append.
//
// NOTE: none of these report errors (get returns a VALUE, set/append are void).
// In-bounds behaviour is tested directly. Out-of-range behaviour is left as
// DISABLED_ hypotheses at the bottom, since it may be undefined in the impl —
// enable them once you've pinned the contract.
//
// Adjust the include path below to match your tree.

extern "C" {
#include "../lib/priv.h"
}

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

// priv.h declares these extern. If you link multiple test files into one
// binary, centralize these (or mark them weak) to avoid duplicate symbols.
__thread char last_err_msg[256] = {0};
bool abort_on_errors = false;

static_assert(sizeof(VALUE) == 8, "VALUE expected to be a 64-bit nanbox");

// Raw bits of a canonical (non-NaN) VALUE, for comparison without operator==.
static uint64_t vbits(VALUE v) {
    uint64_t b;
    std::memcpy(&b, &v, sizeof b);
    return b;
}

// One representative of every storable value kind. NaN is omitted on purpose:
// the array stores raw 8-byte values, so finite reals keep bit-comparison exact.
static std::vector<VALUE> sample_values() {
    return {
        create_value_nil(),
        create_value_bool(true),
        create_value_bool(false),
        create_value_integer(0),
        create_value_integer(-1),
        create_value_integer(INT32_MIN),
        create_value_integer(INT32_MAX),
        create_value_real(0.0),
        create_value_real(3.14),
        create_value_real(-2.5),
        create_value_function_idx(123),
        create_value_native_pointer(nullptr),
        create_value_heap_key(TYC_STRING, 1),
        create_value_heap_key(TYC_ARRAY, 2),
        create_value_heap_key(TYC_TABLE, 3),
    };
}

// =========================================================================
// Lifecycle + length
// =========================================================================

TEST(Array, NewIsEmpty) {
    Array* a = array_new();
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(array_len(a), 0u);
    array_destroy(a);
}

TEST(Array, AppendGrowsLength) {
    Array* a = array_new();
    array_append(a, create_value_integer(10)); EXPECT_EQ(array_len(a), 1u);
    array_append(a, create_value_integer(11)); EXPECT_EQ(array_len(a), 2u);
    array_append(a, create_value_integer(12)); EXPECT_EQ(array_len(a), 3u);
    array_destroy(a);
}

// =========================================================================
// append + get
// =========================================================================

TEST(Array, AppendStoresInOrder) {
    Array* a = array_new();
    for (int i = 0; i < 5; ++i) array_append(a, create_value_integer(i * 100));
    ASSERT_EQ(array_len(a), 5u);
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(value_integer(array_get(a, i)), i * 100) << "pos " << i;
    array_destroy(a);
}

TEST(Array, AppendManyForcesGrowth) {
    Array* a = array_new();
    const int N = 10000;  // exercises several internal reallocs
    for (int i = 0; i < N; ++i) array_append(a, create_value_integer(i));
    ASSERT_EQ(array_len(a), (size_t)N);
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(value_integer(array_get(a, i)), i) << "pos " << i;
    array_destroy(a);
}

TEST(Array, GetReturnsExactBitsAllTypes) {
    Array* a = array_new();
    auto vals = sample_values();
    for (VALUE v : vals) array_append(a, v);
    ASSERT_EQ(array_len(a), vals.size());
    for (size_t i = 0; i < vals.size(); ++i)
        EXPECT_EQ(vbits(array_get(a, i)), vbits(vals[i])) << "pos " << i;
    array_destroy(a);
}

// =========================================================================
// set (in-bounds)
// =========================================================================

TEST(Array, SetWithinBoundsOverwrites) {
    Array* a = array_new();
    for (int i = 0; i < 4; ++i) array_append(a, create_value_integer(i));
    array_set(a, 1, create_value_integer(999));
    EXPECT_EQ(value_integer(array_get(a, 1)), 999);
    EXPECT_EQ(value_integer(array_get(a, 0)), 0);  // neighbours intact
    EXPECT_EQ(value_integer(array_get(a, 2)), 2);
    EXPECT_EQ(array_len(a), 4u);                   // length unchanged
    array_destroy(a);
}

TEST(Array, SetFirstAndLast) {
    Array* a = array_new();
    for (int i = 0; i < 3; ++i) array_append(a, create_value_integer(i));
    array_set(a, 0, create_value_integer(-7));
    array_set(a, 2, create_value_integer(-9));
    EXPECT_EQ(value_integer(array_get(a, 0)), -7);
    EXPECT_EQ(value_integer(array_get(a, 1)), 1);
    EXPECT_EQ(value_integer(array_get(a, 2)), -9);
    array_destroy(a);
}

TEST(Array, SetRoundTripAllTypesAtEveryPosition) {
    auto vals = sample_values();
    Array* a = array_new();
    for (size_t i = 0; i < vals.size(); ++i) array_append(a, create_value_nil());
    for (size_t i = 0; i < vals.size(); ++i) array_set(a, i, vals[i]);
    for (size_t i = 0; i < vals.size(); ++i)
        EXPECT_EQ(vbits(array_get(a, i)), vbits(vals[i])) << "pos " << i;
    array_destroy(a);
}

// =========================================================================
// Open contract questions — DISABLED by default.
// Out-of-range get/set may be undefined behaviour in the implementation;
// running these before confirming the contract could crash the suite.
// Remove the "DISABLED_" prefix to enable.
// =========================================================================

// Hypothesis: reading at or past the logical end yields nil (Lua-like).
TEST(Array, DISABLED_GetOutOfRangeReturnsNil) {
    Array* a = array_new();
    array_append(a, create_value_integer(1));            // len 1
    EXPECT_EQ(value_type(array_get(a, 1)), TYC_NIL);     // one past end
    EXPECT_EQ(value_type(array_get(a, 1000)), TYC_NIL);  // far past end
    array_destroy(a);

    Array* empty = array_new();
    EXPECT_EQ(value_type(array_get(empty, 0)), TYC_NIL); // empty array
    array_destroy(empty);
}

// Hypothesis: setting past the end grows the array, filling the gap with nil.
TEST(Array, DISABLED_SetPastEndGrowsWithNilGap) {
    Array* a = array_new();
    array_append(a, create_value_integer(0));   // len 1
    array_set(a, 3, create_value_integer(30));  // skips indices 1,2
    EXPECT_EQ(array_len(a), 4u);
    EXPECT_EQ(value_integer(array_get(a, 0)), 0);
    EXPECT_EQ(value_type(array_get(a, 1)), TYC_NIL);
    EXPECT_EQ(value_type(array_get(a, 2)), TYC_NIL);
    EXPECT_EQ(value_integer(array_get(a, 3)), 30);
    array_destroy(a);
}
