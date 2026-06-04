// tests-heap.cc — unit tests for the HEAP layer declared in priv.h
//
// Covers: heap_new/destroy, string interning (add/get + constant flag),
//         array/table/native-function add+get, supertable linking,
//         heap_size, heap_should_gc, and heap_gc (mark & sweep).
//
// Most tests use a bare heap_new() for a predictable, empty heap. The two
// supertable behaviours need a VM (table_set/get take a TycheVM*), so they are
// standalone VM-based tests. Tables from heap_add_table are heap-owned — those
// tests let tyc_destroy reclaim them rather than calling table_destroy.
//
// abort_on_errors stays false so the ERROR macro returns TYC_ERR (invalid-key
// tests depend on it). Adjust the include path below to match your tree.

extern "C" {
#include "../lib/priv.h"
}

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

__thread char last_err_msg[256] = {0};
bool abort_on_errors = false;

// A key well past anything these tests allocate — used to probe key validation.
static const HEAP_KEY BOGUS_KEY = 123456789u;

// Distinct native-callback identities for round-trip checks.
static TYC_RESULT native_a(TycheVM*) { return TYC_OK; }
static TYC_RESULT native_b(TycheVM*) { return TYC_ERR; }

class HeapTest : public ::testing::Test {
protected:
    Heap* h = nullptr;
    void SetUp() override    { h = heap_new(); ASSERT_NE(h, nullptr); }
    void TearDown() override { if (h) heap_destroy(h); }
};

// =========================================================================
// Lifecycle + strings (interning)
// =========================================================================

TEST_F(HeapTest, NewIsEmpty) {
    // ASSUMPTION: heap_size counts live objects (not bytes). If it's bytes,
    // adjust the exact-count expectations in this file accordingly.
    EXPECT_EQ(heap_size(h), 0u);
}

TEST_F(HeapTest, AddGetString) {
    HEAP_KEY k = heap_add_string(h, "hello", false);
    const char* s = nullptr;
    ASSERT_EQ(heap_get_string(h, k, &s), TYC_OK);
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "hello");
}

TEST_F(HeapTest, StringInterningReusesKey) {
    HEAP_KEY a = heap_add_string(h, "foo", false);
    HEAP_KEY b = heap_add_string(h, "foo", false);
    HEAP_KEY c = heap_add_string(h, "bar", false);
    EXPECT_EQ(a, b);   // same string → same record
    EXPECT_NE(a, c);   // different string → different record
}

TEST_F(HeapTest, EmptyStringRoundTrip) {
    HEAP_KEY k = heap_add_string(h, "", false);
    const char* s = nullptr;
    ASSERT_EQ(heap_get_string(h, k, &s), TYC_OK);
    ASSERT_NE(s, nullptr);
    EXPECT_STREQ(s, "");
}

TEST_F(HeapTest, GetStringInvalidKeyFails) {
    // Contract: returns TYC_ERR on a key that was never allocated.
    // A crash here means key validation is missing — that's a real finding.
    const char* s = nullptr;
    EXPECT_EQ(heap_get_string(h, BOGUS_KEY, &s), TYC_ERR);
}

TEST_F(HeapTest, DistinctObjectsGrowSize) {
    EXPECT_EQ(heap_size(h), 0u);
    heap_add_string(h, "a", false);
    heap_add_string(h, "b", false);
    heap_add_string(h, "c", false);
    EXPECT_EQ(heap_size(h), 3u);
    heap_add_string(h, "a", false);   // interned dup — no new object
    EXPECT_EQ(heap_size(h), 3u);
}

// =========================================================================
// Arrays
// =========================================================================

TEST_F(HeapTest, AddGetArray) {
    HEAP_KEY k = heap_add_array(h);
    Array* a = nullptr;
    ASSERT_EQ(heap_get_array(h, k, &a), TYC_OK);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(array_len(a), 0u);
}

TEST_F(HeapTest, ArraysAreNotInterned) {
    EXPECT_NE(heap_add_array(h), heap_add_array(h));  // each add is distinct
}

TEST_F(HeapTest, ArrayIsMutableAndPersistent) {
    HEAP_KEY k = heap_add_array(h);
    Array* a = nullptr;
    ASSERT_EQ(heap_get_array(h, k, &a), TYC_OK);
    array_append(a, create_value_integer(99));

    Array* a2 = nullptr;
    ASSERT_EQ(heap_get_array(h, k, &a2), TYC_OK);
    EXPECT_EQ(a2, a);                  // same object across lookups
    ASSERT_EQ(array_len(a2), 1u);
    EXPECT_EQ(value_integer(array_get(a2, 0)), 99);
}

TEST_F(HeapTest, GetArrayInvalidKeyFails) {
    Array* a = nullptr;
    EXPECT_EQ(heap_get_array(h, BOGUS_KEY, &a), TYC_ERR);
}

// =========================================================================
// Tables
// =========================================================================

TEST_F(HeapTest, AddGetTable) {
    HEAP_KEY k = heap_add_table(h);
    Table* t = nullptr;
    ASSERT_EQ(heap_get_table(h, k, &t), TYC_OK);
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(table_len(t), 0u);
    EXPECT_NE(heap_add_table(h), k);  // distinct per add
}

TEST_F(HeapTest, GetTableInvalidKeyFails) {
    Table* t = nullptr;
    EXPECT_EQ(heap_get_table(h, BOGUS_KEY, &t), TYC_ERR);
}

// =========================================================================
// Native functions
// =========================================================================

TEST_F(HeapTest, AddGetNativeFunction) {
    HEAP_KEY ka = heap_add_native_function(h, native_a);
    HEAP_KEY kb = heap_add_native_function(h, native_b);
    EXPECT_NE(ka, kb);

    TYCHE_CB cb = nullptr;
    ASSERT_EQ(heap_get_native_function(h, ka, &cb), TYC_OK);
    EXPECT_EQ(cb, native_a);
    ASSERT_EQ(heap_get_native_function(h, kb, &cb), TYC_OK);
    EXPECT_EQ(cb, native_b);
}

TEST_F(HeapTest, GetNativeFunctionInvalidKeyFails) {
    TYCHE_CB cb = nullptr;
    EXPECT_EQ(heap_get_native_function(h, BOGUS_KEY, &cb), TYC_ERR);
}

// =========================================================================
// Supertable linking — structural (keys only)
// =========================================================================

TEST_F(HeapTest, SetAndRemoveSupertableOk) {
    HEAP_KEY child = heap_add_table(h);
    HEAP_KEY super = heap_add_table(h);
    EXPECT_EQ(heap_set_supertable(h, child, super), TYC_OK);
    EXPECT_EQ(heap_remove_supertable(h, child), TYC_OK);
}

TEST_F(HeapTest, SetSupertableInvalidKeysFail) {
    HEAP_KEY ok = heap_add_table(h);
    EXPECT_EQ(heap_set_supertable(h, BOGUS_KEY, ok),        TYC_ERR);  // bad child
    EXPECT_EQ(heap_set_supertable(h, ok,        BOGUS_KEY), TYC_ERR);  // bad super
}

// =========================================================================
// heap_should_gc — the allocation-pressure trigger
// =========================================================================

TEST_F(HeapTest, ShouldGcFiresUnderAllocationPressure) {
    EXPECT_FALSE(heap_should_gc(h));  // ASSUMPTION: fresh heap is below ceiling

    const int CAP = 100000;  // raise this if your GC ceiling is larger
    bool fired = false;
    for (int i = 0; i < CAP; ++i) {
        heap_add_array(h);
        if (heap_should_gc(h)) { fired = true; break; }
    }
    EXPECT_TRUE(fired) << "heap_should_gc never fired within " << CAP
                       << " allocations";
}

// =========================================================================
// heap_gc — mark & sweep
// =========================================================================

TEST_F(HeapTest, GcKeepsRootedSweepsUnrooted) {
    HEAP_KEY ka = heap_add_array(h);
    HEAP_KEY ks = heap_add_string(h, "gone", false);
    HEAP_KEY kt = heap_add_table(h);
    size_t before = heap_size(h);

    VALUE roots[] = { create_value_heap_key(TYC_ARRAY, ka) };
    heap_gc(h, roots, 1);

    Array* a = nullptr;
    EXPECT_EQ(heap_get_array(h, ka, &a), TYC_OK);    // rooted → survives
    const char* s = nullptr;
    EXPECT_EQ(heap_get_string(h, ks, &s), TYC_ERR);  // unrooted → swept
    Table* t = nullptr;
    EXPECT_EQ(heap_get_table(h, kt, &t), TYC_ERR);   // unrooted → swept
    EXPECT_LT(heap_size(h), before);
}

// The key GC-correctness test: a value reachable only THROUGH a rooted object
// must survive. Mark has to follow references inside heap objects.
TEST_F(HeapTest, GcFollowsReferencesTransitively) {
    HEAP_KEY ka = heap_add_array(h);
    HEAP_KEY ks = heap_add_string(h, "ref", false);

    Array* a = nullptr;
    ASSERT_EQ(heap_get_array(h, ka, &a), TYC_OK);
    array_append(a, create_value_heap_key(TYC_STRING, ks));  // array holds string

    VALUE roots[] = { create_value_heap_key(TYC_ARRAY, ka) };  // root only the array
    heap_gc(h, roots, 1);

    EXPECT_EQ(heap_get_array(h, ka, &a), TYC_OK);    // array survives
    const char* s = nullptr;
    ASSERT_EQ(heap_get_string(h, ks, &s), TYC_OK);   // string survives transitively
    EXPECT_STREQ(s, "ref");
}

TEST_F(HeapTest, GcPinsConstantStrings) {
    HEAP_KEY pinned = heap_add_string(h, "pinned", true);   // constant → never removed
    HEAP_KEY temp   = heap_add_string(h, "temp",   false);  // collectable

    heap_gc(h, nullptr, 0);  // nothing rooted

    const char* s = nullptr;
    ASSERT_EQ(heap_get_string(h, pinned, &s), TYC_OK);  // constant survives
    EXPECT_STREQ(s, "pinned");
    EXPECT_EQ(heap_get_string(h, temp, &s), TYC_ERR);   // non-constant swept
}

TEST_F(HeapTest, GcIgnoresNonCollectableRoots) {
    HEAP_KEY ka = heap_add_array(h);
    VALUE roots[] = {
        create_value_integer(42),                  // non-collectable
        create_value_heap_key(TYC_ARRAY, ka),      // the real root
        create_value_nil(),                        // non-collectable
    };
    heap_gc(h, roots, 3);  // must not choke on the non-heap roots

    Array* a = nullptr;
    EXPECT_EQ(heap_get_array(h, ka, &a), TYC_OK);
}

TEST_F(HeapTest, GcIsIdempotent) {
    HEAP_KEY ka = heap_add_array(h);
    heap_add_string(h, "dead", false);  // unrooted

    VALUE roots[] = { create_value_heap_key(TYC_ARRAY, ka) };
    heap_gc(h, roots, 1);
    size_t after_first = heap_size(h);
    heap_gc(h, roots, 1);               // second pass changes nothing

    EXPECT_EQ(heap_size(h), after_first);
    Array* a = nullptr;
    EXPECT_EQ(heap_get_array(h, ka, &a), TYC_OK);
}

// =========================================================================
// Supertable linking — behavioural (needs a VM for table_set/get)
// =========================================================================

TEST(HeapSupertable, SetSupertableCopiesData) {
    TycheVM* T = tyc_new();
    ASSERT_NE(T, nullptr);
    Heap* h = tyc_heap(T);
    ASSERT_NE(h, nullptr);

    HEAP_KEY ks = heap_add_table(h);  // super
    HEAP_KEY kc = heap_add_table(h);  // child
    Table* super = nullptr;
    Table* child = nullptr;
    ASSERT_EQ(heap_get_table(h, ks, &super), TYC_OK);
    ASSERT_EQ(heap_get_table(h, kc, &child), TYC_OK);

    VALUE key = create_value_heap_key(TYC_STRING, heap_add_string(h, "x", false));
    ASSERT_EQ(table_set(super, key, create_value_integer(1), T), TYC_OK);

    ASSERT_EQ(heap_set_supertable(h, kc, ks), TYC_OK);

    VALUE out;
    ASSERT_EQ(table_get(child, key, &out, T), TYC_OK);
    EXPECT_EQ(value_type(out), TYC_INTEGER);
    EXPECT_EQ(value_integer(out), 1);  // data copied from super at link time

    tyc_destroy(T);  // heap-owned tables reclaimed here — do not table_destroy
}

TEST(HeapSupertable, RemoveSupertableStopsFunctionFallthrough) {
    TycheVM* T = tyc_new();
    ASSERT_NE(T, nullptr);
    Heap* h = tyc_heap(T);
    ASSERT_NE(h, nullptr);

    HEAP_KEY ks = heap_add_table(h);
    HEAP_KEY kc = heap_add_table(h);
    Table* super = nullptr;
    Table* child = nullptr;
    ASSERT_EQ(heap_get_table(h, ks, &super), TYC_OK);
    ASSERT_EQ(heap_get_table(h, kc, &child), TYC_OK);

    VALUE key = create_value_heap_key(TYC_STRING, heap_add_string(h, "f", false));
    ASSERT_EQ(table_set(super, key, create_value_function_idx(5), T), TYC_OK);
    ASSERT_EQ(heap_set_supertable(h, kc, ks), TYC_OK);

    VALUE out;
    ASSERT_EQ(table_get(child, key, &out, T), TYC_OK);
    EXPECT_EQ(value_function_idx(out), 5u);   // function reached via fallthrough

    ASSERT_EQ(heap_remove_supertable(h, kc), TYC_OK);

    // ASSUMPTION (missing key → nil): with the link gone, the function no
    // longer resolves through the (former) super.
    ASSERT_EQ(table_get(child, key, &out, T), TYC_OK);
    EXPECT_EQ(value_type(out), TYC_NIL);

    tyc_destroy(T);
}
