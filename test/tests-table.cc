// tests-table.cc — unit tests for the HEAP TABLE layer declared in priv.h
//
// Covers: table_new/destroy, table_len, table_get/set/del, table_has_key,
//         table_next (iteration), table_setsuper (supertable inheritance).
//
// Tables are NOT standalone: table_new takes a Heap*, and get/set/del take a
// TycheVM* (needed for string-key interning and overloaded __hash/__eq). The
// fixture therefore builds a real VM and uses its heap. None of these tests
// trigger operator overloading, so primitive and string keys suffice.
//
// abort_on_errors MUST stay false so the ERROR macro returns TYC_ERR instead
// of abort()-ing — the invalid-key tests rely on it.
//
// Adjust the include path below to match your tree.

extern "C" {
#include "../lib/priv.h"
}

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <map>

__thread char last_err_msg[256] = {0};
bool abort_on_errors = false;

static_assert(sizeof(VALUE) == 8, "VALUE expected to be a 64-bit nanbox");

static uint64_t vbits(VALUE v) {
    uint64_t b;
    std::memcpy(&b, &v, sizeof b);
    return b;
}

class TableTest : public ::testing::Test {
protected:
    TycheVM* T = nullptr;
    Heap*    h = nullptr;
    Table*   t = nullptr;

    void SetUp() override {
        T = tyc_new();
        ASSERT_NE(T, nullptr);
        h = tyc_heap(T);
        ASSERT_NE(h, nullptr);
        t = table_new(h);
        ASSERT_NE(t, nullptr);
    }
    void TearDown() override {
        if (t) table_destroy(t);   // destroy table before the heap it borrows
        if (T) tyc_destroy(T);
    }

    // Intern a string into the heap and wrap it as a string VALUE.
    VALUE str(const char* s) {
        return create_value_heap_key(TYC_STRING, heap_add_string(h, s, false));
    }
};

// =========================================================================
// Basic set / get / has_key / len
// =========================================================================

TEST_F(TableTest, NewIsEmpty) {
    EXPECT_EQ(table_len(t), 0u);
}

TEST_F(TableTest, SetGetInteger) {
    ASSERT_EQ(table_set(t, create_value_integer(1), create_value_integer(100), T), TYC_OK);
    EXPECT_TRUE(table_has_key(t, create_value_integer(1)));
    VALUE out;
    ASSERT_EQ(table_get(t, create_value_integer(1), &out, T), TYC_OK);
    EXPECT_EQ(value_type(out), TYC_INTEGER);
    EXPECT_EQ(value_integer(out), 100);
    EXPECT_EQ(table_len(t), 1u);
}

TEST_F(TableTest, SetGetString) {
    ASSERT_EQ(table_set(t, str("name"), str("tyche"), T), TYC_OK);
    VALUE out;
    ASSERT_EQ(table_get(t, str("name"), &out, T), TYC_OK);
    EXPECT_EQ(value_type(out), TYC_STRING);
    EXPECT_EQ(vbits(out), vbits(str("tyche")));  // interned → same heap key
    EXPECT_TRUE(table_has_key(t, str("name")));
}

TEST_F(TableTest, SetGetBooleanKeysAreDistinct) {
    ASSERT_EQ(table_set(t, create_value_bool(true),  create_value_integer(1), T), TYC_OK);
    ASSERT_EQ(table_set(t, create_value_bool(false), create_value_integer(0), T), TYC_OK);
    EXPECT_EQ(table_len(t), 2u);
    VALUE out;
    ASSERT_EQ(table_get(t, create_value_bool(true), &out, T), TYC_OK);
    EXPECT_EQ(value_integer(out), 1);
    ASSERT_EQ(table_get(t, create_value_bool(false), &out, T), TYC_OK);
    EXPECT_EQ(value_integer(out), 0);
}

TEST_F(TableTest, OverwriteKeepsLength) {
    ASSERT_EQ(table_set(t, create_value_integer(1), create_value_integer(100), T), TYC_OK);
    ASSERT_EQ(table_set(t, create_value_integer(1), create_value_integer(200), T), TYC_OK);
    VALUE out;
    ASSERT_EQ(table_get(t, create_value_integer(1), &out, T), TYC_OK);
    EXPECT_EQ(value_integer(out), 200);
    EXPECT_EQ(table_len(t), 1u);  // overwrite, not insert
}

TEST_F(TableTest, MixedKeyTypesCoexist) {
    ASSERT_EQ(table_set(t, create_value_integer(7), create_value_integer(1), T), TYC_OK);
    ASSERT_EQ(table_set(t, str("k"),                create_value_integer(2), T), TYC_OK);
    ASSERT_EQ(table_set(t, create_value_bool(true), create_value_integer(3), T), TYC_OK);
    EXPECT_EQ(table_len(t), 3u);
    VALUE out;
    ASSERT_EQ(table_get(t, create_value_integer(7), &out, T), TYC_OK); EXPECT_EQ(value_integer(out), 1);
    ASSERT_EQ(table_get(t, str("k"),                &out, T), TYC_OK); EXPECT_EQ(value_integer(out), 2);
    ASSERT_EQ(table_get(t, create_value_bool(true), &out, T), TYC_OK); EXPECT_EQ(value_integer(out), 3);
}

// String interning means equal strings are the same key even via different
// VALUEs — setting through one must be visible through another.
TEST_F(TableTest, StringKeysInternToSameEntry) {
    VALUE k1 = str("dup");
    VALUE k2 = str("dup");
    EXPECT_EQ(vbits(k1), vbits(k2));  // interning precondition
    ASSERT_EQ(table_set(t, k1, create_value_integer(42), T), TYC_OK);
    VALUE out;
    ASSERT_EQ(table_get(t, k2, &out, T), TYC_OK);
    EXPECT_EQ(value_integer(out), 42);
    EXPECT_EQ(table_len(t), 1u);
}

// ASSUMPTION (Lua-like): a missing key of a VALID type yields TYC_OK + nil,
// not an error. Flip if your table_get errors on absent keys.
TEST_F(TableTest, MissingKeyReturnsNil) {
    VALUE out = create_value_integer(123);  // sentinel that must be overwritten
    EXPECT_EQ(table_get(t, create_value_integer(42), &out, T), TYC_OK);
    EXPECT_EQ(value_type(out), TYC_NIL);
    EXPECT_FALSE(table_has_key(t, create_value_integer(42)));
}

// =========================================================================
// Invalid key types — reals, arrays, nils cannot be keys (per VM.md)
// =========================================================================

TEST_F(TableTest, InvalidKeyTypesRejectedOnSet) {
    VALUE arr = create_value_heap_key(TYC_ARRAY, heap_add_array(h));
    EXPECT_EQ(table_set(t, create_value_nil(),    create_value_integer(1), T), TYC_ERR);
    EXPECT_EQ(table_set(t, create_value_real(1.5), create_value_integer(1), T), TYC_ERR);
    EXPECT_EQ(table_set(t, arr,                    create_value_integer(1), T), TYC_ERR);
    EXPECT_EQ(table_len(t), 0u);  // nothing inserted
}

// ASSUMPTION: get with an invalid-type key errors (can't be hashed). If your
// impl short-circuits invalid keys to nil instead, change these to TYC_OK+nil.
TEST_F(TableTest, InvalidKeyTypesRejectedOnGet) {
    VALUE arr = create_value_heap_key(TYC_ARRAY, heap_add_array(h));
    VALUE out;
    EXPECT_EQ(table_get(t, create_value_nil(),     &out, T), TYC_ERR);
    EXPECT_EQ(table_get(t, create_value_real(1.5), &out, T), TYC_ERR);
    EXPECT_EQ(table_get(t, arr,                     &out, T), TYC_ERR);
}

// =========================================================================
// Delete
// =========================================================================

TEST_F(TableTest, DelRemovesKey) {
    ASSERT_EQ(table_set(t, str("a"), create_value_integer(1), T), TYC_OK);
    ASSERT_EQ(table_set(t, str("b"), create_value_integer(2), T), TYC_OK);
    ASSERT_EQ(table_len(t), 2u);

    ASSERT_EQ(table_del(t, str("a"), T), TYC_OK);
    EXPECT_FALSE(table_has_key(t, str("a")));
    EXPECT_EQ(table_len(t), 1u);
    // surviving key untouched
    VALUE out;
    ASSERT_EQ(table_get(t, str("b"), &out, T), TYC_OK);
    EXPECT_EQ(value_integer(out), 2);
}

// Deleting an absent key must not corrupt the table. The return code for this
// case is unspecified, so we don't assert it — only the observable state.
TEST_F(TableTest, DelMissingKeyLeavesTableIntact) {
    ASSERT_EQ(table_set(t, str("a"), create_value_integer(1), T), TYC_OK);
    table_del(t, str("zzz"), T);  // not present
    EXPECT_EQ(table_len(t), 1u);
    EXPECT_TRUE(table_has_key(t, str("a")));
}

// =========================================================================
// Iteration — table_next (stateless Lua-style protocol: feed last key back)
// =========================================================================

TEST_F(TableTest, NextIteratesEveryPairOnce) {
    const int N = 50;
    for (int i = 1; i <= N; ++i)
        ASSERT_EQ(table_set(t, create_value_integer(i), create_value_integer(i * 10), T), TYC_OK);

    std::map<int32_t, int32_t> seen;
    VALUE key = create_value_nil();  // nil → start from the first pair
    VALUE ok = create_value_nil(), ov;
    int guard = 0;
    // Terminate on either convention: returns false, or returns a nil key.
    while (table_next(t, key, &ok, &ov) && value_type(ok) != TYC_NIL) {
        ASSERT_LT(guard++, N + 5) << "table_next did not terminate";
        ASSERT_EQ(value_type(ok), TYC_INTEGER);
        seen[value_integer(ok)] = value_integer(ov);
        key = ok;  // advance
    }
    EXPECT_EQ(seen.size(), (size_t)N);
    for (int i = 1; i <= N; ++i) {
        ASSERT_TRUE(seen.count(i)) << "missing key " << i;
        EXPECT_EQ(seen[i], i * 10);
    }
}

TEST_F(TableTest, NextOnEmptyHasNoPairs) {
    VALUE ok = create_value_integer(999), ov;
    bool more = table_next(t, create_value_nil(), &ok, &ov);
    EXPECT_TRUE(!more || value_type(ok) == TYC_NIL);
}

// =========================================================================
// Supertables (table_setsuper)
//
// VM.md rules:
//   * On link, super's DATA keys/values are shallow-copied into the child
//     (functions/overloads are NOT copied).
//   * Missing key in child falls through to super only if the field is a
//     function (this is how method/overload inheritance works).
//   * Data is frozen at link time; later super-data changes don't propagate,
//     but later super-FUNCTION changes do (live fallthrough).
// =========================================================================

TEST_F(TableTest, SuperDataIsCopiedAtLink) {
    Table* super = table_new(h);
    Table* child = table_new(h);
    ASSERT_EQ(table_set(super, str("x"), create_value_integer(1), T), TYC_OK);
    ASSERT_EQ(table_set(child, str("y"), create_value_integer(2), T), TYC_OK);

    table_setsuper(child, super);

    VALUE out;
    ASSERT_EQ(table_get(child, str("x"), &out, T), TYC_OK);  // copied from super
    EXPECT_EQ(value_integer(out), 1);
    ASSERT_EQ(table_get(child, str("y"), &out, T), TYC_OK);  // own key intact
    EXPECT_EQ(value_integer(out), 2);
    // ASSUMPTION: copied super-data counts toward the child's length.
    EXPECT_EQ(table_len(child), 2u);

    table_destroy(child);
    table_destroy(super);
}

TEST_F(TableTest, SuperDataChangeDoesNotPropagateAfterLink) {
    Table* super = table_new(h);
    Table* child = table_new(h);
    ASSERT_EQ(table_set(super, str("x"), create_value_integer(1), T), TYC_OK);
    table_setsuper(child, super);

    ASSERT_EQ(table_set(super, str("x"), create_value_integer(99), T), TYC_OK);  // after link

    VALUE out;
    ASSERT_EQ(table_get(child, str("x"), &out, T), TYC_OK);
    EXPECT_EQ(value_integer(out), 1);  // child kept the link-time snapshot

    table_destroy(child);
    table_destroy(super);
}

TEST_F(TableTest, SuperFunctionFallsThrough) {
    Table* super = table_new(h);
    Table* child = table_new(h);
    ASSERT_EQ(table_set(super, str("f"), create_value_function_idx(7), T), TYC_OK);
    table_setsuper(child, super);

    VALUE out;
    ASSERT_EQ(table_get(child, str("f"), &out, T), TYC_OK);  // not copied, fell through
    EXPECT_EQ(value_type(out), TYC_FUNCTION);
    EXPECT_EQ(value_function_idx(out), 7u);

    table_destroy(child);
    table_destroy(super);
}

TEST_F(TableTest, SuperFunctionUpdatePropagates) {
    Table* super = table_new(h);
    Table* child = table_new(h);
    ASSERT_EQ(table_set(super, str("f"), create_value_function_idx(7), T), TYC_OK);
    table_setsuper(child, super);

    ASSERT_EQ(table_set(super, str("f"), create_value_function_idx(8), T), TYC_OK);  // after link

    VALUE out;
    ASSERT_EQ(table_get(child, str("f"), &out, T), TYC_OK);
    EXPECT_EQ(value_function_idx(out), 8u);  // functions resolve live through super

    table_destroy(child);
    table_destroy(super);
}

TEST_F(TableTest, SuperChainFunctionFallsThrough) {
    Table* gp = table_new(h);   // grandparent
    Table* pa = table_new(h);   // parent
    Table* ch = table_new(h);   // child
    ASSERT_EQ(table_set(gp, str("g"), create_value_function_idx(11), T), TYC_OK);
    table_setsuper(pa, gp);
    table_setsuper(ch, pa);

    VALUE out;
    ASSERT_EQ(table_get(ch, str("g"), &out, T), TYC_OK);  // two hops up the chain
    EXPECT_EQ(value_type(out), TYC_FUNCTION);
    EXPECT_EQ(value_function_idx(out), 11u);

    table_destroy(ch);   // destroy children before their supers
    table_destroy(pa);
    table_destroy(gp);
}
