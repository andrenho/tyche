// tests-stack.cc — unit tests for the STACK layer declared in priv.h
//
// Covers: stack_new/destroy, push/peek/pop, size, at/set (FP-relative
//         indexing), the frame-pointer sub-stack (top_fp/push_fp/pop_fp/
//         fp_level), and stack_collectable_array.
//
// INDEXING ASSUMPTIONS (from VM.md):
//   * Positive indices are 0-based, counted from the FP base upward
//     (the doc diagram labels visible slots "00 01 02 03"). If your VM is
//     1-based Lua-style, shift every positive index in these tests by +1.
//   * Negative indices count from the top: -1 == top, -2 == one below, ...
//   * Slots below the current FP are invisible to at/set.
//
// NOTE: adjust the include path below to match your tree.

extern "C" {
#include "../lib/priv.h"
}

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <set>

// priv.h declares these extern; stack.c likely references them via ERROR().
// If multiple test files are linked into ONE binary, centralize these two
// definitions (or mark them __attribute__((weak))) to avoid duplicate symbols.
__thread char last_err_msg[256] = {0};
bool abort_on_errors = false;

static_assert(sizeof(VALUE) == 8, "VALUE expected to be a 64-bit nanbox");

// Raw bits of a (canonical, non-NaN) VALUE — fine for the integer/heap-key
// values used here; lets us compare values without a public operator==.
static uint64_t vbits(VALUE v) {
    uint64_t b;
    std::memcpy(&b, &v, sizeof b);
    return b;
}

// Convenience: push an integer and assert success.
static void push_int(Stack* s, int32_t n) {
    ASSERT_EQ(stack_push(s, create_value_integer(n)), TYC_OK);
}

// =========================================================================
// Lifecycle + basic push/peek/pop/size
// =========================================================================

TEST(Stack, NewIsEmpty) {
    Stack* s = stack_new();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(stack_size(s), 0u);
    EXPECT_EQ(stack_top_fp(s), 0u);  // base frame starts at position 0
    stack_destroy(s);
}

TEST(Stack, PushIncrementsSize) {
    Stack* s = stack_new();
    push_int(s, 10);  EXPECT_EQ(stack_size(s), 1u);
    push_int(s, 11);  EXPECT_EQ(stack_size(s), 2u);
    push_int(s, 12);  EXPECT_EQ(stack_size(s), 3u);
    stack_destroy(s);
}

TEST(Stack, PeekDoesNotPop) {
    Stack* s = stack_new();
    push_int(s, 42);
    VALUE v;
    ASSERT_EQ(stack_peek(s, &v), TYC_OK);
    EXPECT_EQ(value_integer(v), 42);
    EXPECT_EQ(stack_size(s), 1u);  // peek leaves it in place
    ASSERT_EQ(stack_peek(s, &v), TYC_OK);  // still there
    EXPECT_EQ(value_integer(v), 42);
    stack_destroy(s);
}

TEST(Stack, PopReturnsLIFO) {
    Stack* s = stack_new();
    push_int(s, 0);
    push_int(s, 1);
    push_int(s, 2);
    VALUE v;
    ASSERT_EQ(stack_pop(s, &v), TYC_OK); EXPECT_EQ(value_integer(v), 2);
    ASSERT_EQ(stack_pop(s, &v), TYC_OK); EXPECT_EQ(value_integer(v), 1);
    ASSERT_EQ(stack_pop(s, &v), TYC_OK); EXPECT_EQ(value_integer(v), 0);
    EXPECT_EQ(stack_size(s), 0u);
    stack_destroy(s);
}

TEST(Stack, PeekEmptyFails) {
    Stack* s = stack_new();
    VALUE v;
    EXPECT_EQ(stack_peek(s, &v), TYC_ERR);
    EXPECT_EQ(stack_size(s), 0u);
    stack_destroy(s);
}

TEST(Stack, PopEmptyFails) {
    Stack* s = stack_new();
    VALUE v;
    EXPECT_EQ(stack_pop(s, &v), TYC_ERR);
    EXPECT_EQ(stack_size(s), 0u);
    // and underflow after draining a non-empty stack
    push_int(s, 7);
    ASSERT_EQ(stack_pop(s, &v), TYC_OK);
    EXPECT_EQ(stack_pop(s, &v), TYC_ERR);
    stack_destroy(s);
}

TEST(Stack, GrowsUnderManyPushes) {
    Stack* s = stack_new();
    const int N = 10000;  // forces several internal reallocs
    for (int i = 0; i < N; ++i) push_int(s, i);
    EXPECT_EQ(stack_size(s), (size_t)N);
    for (int i = N - 1; i >= 0; --i) {
        VALUE v;
        ASSERT_EQ(stack_pop(s, &v), TYC_OK) << "at i=" << i;
        EXPECT_EQ(value_integer(v), i) << "at i=" << i;
    }
    EXPECT_EQ(stack_size(s), 0u);
    stack_destroy(s);
}

// =========================================================================
// stack_at / stack_set — FP-relative indexing at the base frame
// (FP at 0, so relative == absolute here)
// =========================================================================

TEST(Stack, AtPositiveIsZeroBasedFromBase) {
    Stack* s = stack_new();
    push_int(s, 100);  // index 0
    push_int(s, 101);  // index 1
    push_int(s, 102);  // index 2
    push_int(s, 103);  // index 3 (top)
    VALUE v;
    ASSERT_EQ(stack_at(s, 0, &v), TYC_OK); EXPECT_EQ(value_integer(v), 100);
    ASSERT_EQ(stack_at(s, 1, &v), TYC_OK); EXPECT_EQ(value_integer(v), 101);
    ASSERT_EQ(stack_at(s, 2, &v), TYC_OK); EXPECT_EQ(value_integer(v), 102);
    ASSERT_EQ(stack_at(s, 3, &v), TYC_OK); EXPECT_EQ(value_integer(v), 103);
    stack_destroy(s);
}

TEST(Stack, AtNegativeCountsFromTop) {
    Stack* s = stack_new();
    push_int(s, 100);
    push_int(s, 101);
    push_int(s, 102);
    push_int(s, 103);  // top
    VALUE v;
    ASSERT_EQ(stack_at(s, -1, &v), TYC_OK); EXPECT_EQ(value_integer(v), 103);
    ASSERT_EQ(stack_at(s, -2, &v), TYC_OK); EXPECT_EQ(value_integer(v), 102);
    ASSERT_EQ(stack_at(s, -3, &v), TYC_OK); EXPECT_EQ(value_integer(v), 101);
    ASSERT_EQ(stack_at(s, -4, &v), TYC_OK); EXPECT_EQ(value_integer(v), 100);
    stack_destroy(s);
}

TEST(Stack, AtOutOfRangeFails) {
    Stack* s = stack_new();
    push_int(s, 100);
    push_int(s, 101);
    push_int(s, 102);
    push_int(s, 103);  // size 4 → valid: 0..3 and -1..-4
    VALUE v;
    EXPECT_EQ(stack_at(s, 4, &v), TYC_ERR);
    EXPECT_EQ(stack_at(s, 100, &v), TYC_ERR);
    EXPECT_EQ(stack_at(s, -5, &v), TYC_ERR);
    EXPECT_EQ(stack_at(s, -100, &v), TYC_ERR);
    stack_destroy(s);
}

TEST(Stack, AtEmptyFails) {
    Stack* s = stack_new();
    VALUE v;
    EXPECT_EQ(stack_at(s, 0, &v), TYC_ERR);
    EXPECT_EQ(stack_at(s, -1, &v), TYC_ERR);
    stack_destroy(s);
}

TEST(Stack, SetThenAtPositive) {
    Stack* s = stack_new();
    push_int(s, 100);
    push_int(s, 101);
    push_int(s, 102);
    push_int(s, 103);
    ASSERT_EQ(stack_set(s, 1, create_value_integer(999)), TYC_OK);
    VALUE v;
    ASSERT_EQ(stack_at(s, 1, &v), TYC_OK); EXPECT_EQ(value_integer(v), 999);
    // neighbours untouched
    ASSERT_EQ(stack_at(s, 0, &v), TYC_OK); EXPECT_EQ(value_integer(v), 100);
    ASSERT_EQ(stack_at(s, 2, &v), TYC_OK); EXPECT_EQ(value_integer(v), 102);
    EXPECT_EQ(stack_size(s), 4u);  // set does not change size
    stack_destroy(s);
}

TEST(Stack, SetNegativeIndexHitsTop) {
    Stack* s = stack_new();
    push_int(s, 100);
    push_int(s, 101);
    push_int(s, 102);
    push_int(s, 103);
    ASSERT_EQ(stack_set(s, -1, create_value_integer(555)), TYC_OK);
    VALUE v;
    ASSERT_EQ(stack_at(s, -1, &v), TYC_OK); EXPECT_EQ(value_integer(v), 555);
    ASSERT_EQ(stack_at(s, 3, &v), TYC_OK);  EXPECT_EQ(value_integer(v), 555);
    stack_destroy(s);
}

TEST(Stack, SetOutOfRangeFails) {
    Stack* s = stack_new();
    push_int(s, 100);
    push_int(s, 101);
    EXPECT_EQ(stack_set(s, 2, create_value_integer(1)), TYC_ERR);
    EXPECT_EQ(stack_set(s, -3, create_value_integer(1)), TYC_ERR);
    stack_destroy(s);
}

// =========================================================================
// Frame pointer sub-stack
// =========================================================================

TEST(Stack, PushFpRaisesBaseAndHidesLowerSlots) {
    Stack* s = stack_new();
    push_int(s, 10);
    push_int(s, 11);
    push_int(s, 12);          // base frame: abs 0,1,2
    size_t lvl0 = stack_fp_level(s);

    ASSERT_EQ(stack_push_fp(s), TYC_OK);
    EXPECT_EQ(stack_top_fp(s), 3u);            // new frame starts at abs 3
    EXPECT_EQ(stack_fp_level(s), lvl0 + 1);

    // empty frame: nothing visible yet
    VALUE v;
    EXPECT_EQ(stack_at(s, 0, &v), TYC_ERR);
    EXPECT_EQ(stack_at(s, -1, &v), TYC_ERR);

    push_int(s, 20);          // frame-relative index 0
    push_int(s, 21);          // frame-relative index 1
    ASSERT_EQ(stack_at(s, 0, &v), TYC_OK);  EXPECT_EQ(value_integer(v), 20);
    ASSERT_EQ(stack_at(s, 1, &v), TYC_OK);  EXPECT_EQ(value_integer(v), 21);
    ASSERT_EQ(stack_at(s, -1, &v), TYC_OK); EXPECT_EQ(value_integer(v), 21);
    ASSERT_EQ(stack_at(s, -2, &v), TYC_OK); EXPECT_EQ(value_integer(v), 20);

    // index 2 / -3 would reach into the hidden base frame → out of range
    EXPECT_EQ(stack_at(s, 2, &v), TYC_ERR);
    EXPECT_EQ(stack_at(s, -3, &v), TYC_ERR);

    // ASSUMPTION: stack_size is FP-relative (visible slots only).
    // If your stack_size is absolute, expect 5 here instead of 2.
    EXPECT_EQ(stack_size(s), 2u);

    stack_destroy(s);
}

TEST(Stack, PopFpRestoresVisibilityWithoutTruncating) {
    Stack* s = stack_new();
    push_int(s, 10);
    push_int(s, 11);
    push_int(s, 12);
    size_t lvl0 = stack_fp_level(s);

    ASSERT_EQ(stack_push_fp(s), TYC_OK);
    push_int(s, 20);
    push_int(s, 21);

    ASSERT_EQ(stack_pop_fp(s), TYC_OK);
    EXPECT_EQ(stack_top_fp(s), 0u);
    EXPECT_EQ(stack_fp_level(s), lvl0);

    // ASSUMPTION: pop_fp leaves the value stack intact, so all 5 are visible.
    EXPECT_EQ(stack_size(s), 5u);
    VALUE v;
    ASSERT_EQ(stack_at(s, 0, &v), TYC_OK); EXPECT_EQ(value_integer(v), 10);
    ASSERT_EQ(stack_at(s, 2, &v), TYC_OK); EXPECT_EQ(value_integer(v), 12);
    ASSERT_EQ(stack_at(s, 3, &v), TYC_OK); EXPECT_EQ(value_integer(v), 20);
    ASSERT_EQ(stack_at(s, 4, &v), TYC_OK); EXPECT_EQ(value_integer(v), 21);
    stack_destroy(s);
}

TEST(Stack, FpLevelTracksNesting) {
    Stack* s = stack_new();
    size_t lvl0 = stack_fp_level(s);
    ASSERT_EQ(stack_push_fp(s), TYC_OK); EXPECT_EQ(stack_fp_level(s), lvl0 + 1);
    ASSERT_EQ(stack_push_fp(s), TYC_OK); EXPECT_EQ(stack_fp_level(s), lvl0 + 2);
    ASSERT_EQ(stack_pop_fp(s), TYC_OK);  EXPECT_EQ(stack_fp_level(s), lvl0 + 1);
    ASSERT_EQ(stack_pop_fp(s), TYC_OK);  EXPECT_EQ(stack_fp_level(s), lvl0);
    stack_destroy(s);
}

TEST(Stack, PopFpBelowBaseFails) {
    // ASSUMPTION: a push_fp must precede every pop_fp; popping the base frame
    // is an error. Remove/adjust if your VM keeps an implicit poppable frame.
    Stack* s = stack_new();
    ASSERT_EQ(stack_push_fp(s), TYC_OK);
    ASSERT_EQ(stack_pop_fp(s), TYC_OK);
    EXPECT_EQ(stack_pop_fp(s), TYC_ERR);
    stack_destroy(s);
}

// =========================================================================
// stack_collectable_array — GC root set
// =========================================================================

TEST(Stack, CollectableArrayReturnsOnlyHeapValues) {
    Stack* s = stack_new();
    // Mix collectable (heap-keyed) and non-collectable values.
    VALUE str = create_value_heap_key(TYC_STRING, 1);
    VALUE tbl = create_value_heap_key(TYC_TABLE, 2);
    VALUE arr = create_value_heap_key(TYC_ARRAY, 3);
    ASSERT_EQ(stack_push(s, create_value_integer(7)), TYC_OK);
    ASSERT_EQ(stack_push(s, str), TYC_OK);
    ASSERT_EQ(stack_push(s, create_value_nil()), TYC_OK);
    ASSERT_EQ(stack_push(s, tbl), TYC_OK);
    ASSERT_EQ(stack_push(s, create_value_bool(true)), TYC_OK);
    ASSERT_EQ(stack_push(s, arr), TYC_OK);

    VALUE* out = nullptr;
    size_t n = stack_collectable_array(s, &out);
    EXPECT_EQ(n, 3u);
    if (n > 0) ASSERT_NE(out, nullptr);

    // Order is not specified, so compare as a set; every returned value must
    // be collectable and must be one we pushed.
    std::multiset<uint64_t> got, want = {vbits(str), vbits(tbl), vbits(arr)};
    for (size_t i = 0; i < n; ++i) {
        EXPECT_TRUE(type_is_collectable(value_type(out[i])))
            << "non-collectable value returned at index " << i;
        got.insert(vbits(out[i]));
    }
    EXPECT_EQ(got, want);

    // Ownership of `out` is unspecified by the header. If it is caller-owned
    // (heap-allocated), add: free(out);  — an ASan leak here tells you so.
    stack_destroy(s);
}

TEST(Stack, CollectableArrayEmptyWhenNoneCollectable) {
    Stack* s = stack_new();
    ASSERT_EQ(stack_push(s, create_value_integer(1)), TYC_OK);
    ASSERT_EQ(stack_push(s, create_value_nil()), TYC_OK);
    ASSERT_EQ(stack_push(s, create_value_bool(false)), TYC_OK);
    ASSERT_EQ(stack_push(s, create_value_real(3.5)), TYC_OK);

    VALUE* out = nullptr;
    EXPECT_EQ(stack_collectable_array(s, &out), 0u);
    stack_destroy(s);
}

TEST(Stack, CollectableArraySpansAllFrames) {
    // GC must root the whole stack, not just the visible frame, so collectables
    // below the current FP must still be reported.
    Stack* s = stack_new();
    VALUE a = create_value_heap_key(TYC_STRING, 10);  // base frame
    VALUE b = create_value_heap_key(TYC_TABLE, 11);   // inner frame
    ASSERT_EQ(stack_push(s, a), TYC_OK);
    ASSERT_EQ(stack_push_fp(s), TYC_OK);
    ASSERT_EQ(stack_push(s, b), TYC_OK);

    VALUE* out = nullptr;
    size_t n = stack_collectable_array(s, &out);
    EXPECT_EQ(n, 2u) << "collectables below the FP must still be rooted";
    if (n > 0) ASSERT_NE(out, nullptr);
    std::multiset<uint64_t> got, want = {vbits(a), vbits(b)};
    for (size_t i = 0; i < n; ++i) got.insert(vbits(out[i]));
    EXPECT_EQ(got, want);
    stack_destroy(s);
}
