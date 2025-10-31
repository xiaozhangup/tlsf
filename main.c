#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Memory pool: 8 MiB
#define POOL_SIZE (8 * 1024 * 1024)
char __attribute__((__aligned__(4096))) my_mem_pool[POOL_SIZE];

// Function prototypes (to be implemented)
void __attribute_noinline__ my_init();
void * __attribute_noinline__ my_malloc(size_t size);
void __attribute_noinline__ my_free(void *ptr);

// -------------------------------
// Helper: Check if ptr is within pool
// -------------------------------
static int in_pool(void *ptr) {
  return (ptr >= (void *)my_mem_pool) &&
         (ptr < (void *)(my_mem_pool + POOL_SIZE));
}

// -------------------------------
// Test utilities
// -------------------------------
static int test_passed = 0;
static int test_failed = 0;

#define RUN_TEST(test_fn)                                                      \
  do {                                                                         \
    printf("Running %s... ", #test_fn);                                        \
    if (test_fn()) {                                                           \
      printf("PASSED\n");                                                      \
      test_passed++;                                                           \
    } else {                                                                   \
      printf("FAILED\n");                                                      \
      test_failed++;                                                           \
    }                                                                          \
  } while (0)

// -------------------------------
// Test 1: Basic allocation and free
// -------------------------------
int test_basic() {
  void *p1 = my_malloc(100);
  if (!p1 || !in_pool(p1))
    return 0;
  memset(p1, 0xAB, 100);

  void *p2 = my_malloc(200);
  if (!p2 || !in_pool(p2) || p2 <= p1)
    return 0;

  my_free(p1);
  my_free(p2);
  return 1;
}

// -------------------------------
// Test 2: Zero-size allocation
// -------------------------------
int test_zero_size() {
  void *p = my_malloc(0);
  // Acceptable: either returns NULL or valid pointer
  // But must not crash. We allow either behavior.
  // However, if it returns non-NULL, it must be in pool.
  if (p && !in_pool(p))
    return 0;
  if (p)
    my_free(p);
  return 1;
}

// -------------------------------
// Test 3: Maximum size (4096 bytes)
// -------------------------------
int test_max_size() {
  void *p = my_malloc(4096);
  if (!p || !in_pool(p))
    return 0;
  memset(p, 0xCD, 4096);
  my_free(p);
  return 1;
}

// -------------------------------
// Test 4: Oversized allocation (>4096)
// -------------------------------
int test_oversize() {
  void *p = my_malloc(4097);
  // Should return NULL
  if (p != NULL)
    return 0;
  return 1;
}

// -------------------------------
// Test 5: Free NULL
// -------------------------------
int test_free_null() {
  // Should not crash
  my_free(NULL);
  return 1;
}

// -------------------------------
// Test 6: Double free detection (basic)
// -------------------------------
int test_double_free() {
  void *p = my_malloc(128);
  if (!p)
    return 0;
  my_free(p);
  // Double free should not crash (graceful handling preferred)
  // We don't require error reporting, just no crash.
  my_free(p);
  return 1;
}

// -------------------------------
// Test 7: Use-after-free detection via pattern
// -------------------------------
int test_use_after_free() {
  void *p = my_malloc(64);
  if (!p)
    return 0;
  memset(p, 0xEF, 64);
  my_free(p);
  // Optional: allocator may poison memory
  // We just check it doesn't crash on subsequent alloc
  void *q = my_malloc(64);
  if (!q)
    return 0;
  my_free(q);
  return 1;
}

// -------------------------------
// Test 8: Fragmentation & reuse
// -------------------------------
int test_fragmentation() {
  const int N = 1000;
  void *ptrs[N];
  // Allocate many small blocks
  for (int i = 0; i < N; i++) {
    ptrs[i] = my_malloc(64);
    if (!ptrs[i])
      return 0;
    *(int *)ptrs[i] = i;
  }
  // Free odd indices
  for (int i = 1; i < N; i += 2) {
    my_free(ptrs[i]);
    ptrs[i] = NULL;
  }
  // Allocate again — should reuse freed space
  for (int i = 1; i < N; i += 2) {
    ptrs[i] = my_malloc(64);
    if (!ptrs[i])
      return 0;
    *(int *)ptrs[i] = i + 1000;
  }
  // Validate data
  for (int i = 0; i < N; i++) {
    if (*(int *)ptrs[i] != (i % 2 == 0 ? i : i + 1000))
      return 0;
  }
  // Clean up
  for (int i = 0; i < N; i++) {
    my_free(ptrs[i]);
  }
  return 1;
}

// -------------------------------
// Test 9: Alignment (at least 8-byte)
// -------------------------------
int test_alignment() {
  for (size_t s = 1; s <= 128; s++) {
    void *p = my_malloc(s);
    if (!p)
      return 0;
    if (((uintptr_t)p) % 8 != 0) {
      my_free(p);
      return 0;
    }
    my_free(p);
  }
  return 1;
}

// -------------------------------
// Test 10: Performance benchmark
// -------------------------------
int test_performance() {
  const int rounds = 10000;
  const int allocs_per_round = 1000;
  size_t sizes[10] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

  clock_t start = clock();
  for (int r = 0; r < rounds; r++) {
    void *ptrs[allocs_per_round];
    // Allocate
    for (int i = 0; i < allocs_per_round; i++) {
      size_t sz = sizes[i % 10];
      ptrs[i] = my_malloc(sz);
      if (!ptrs[i]) {
        fprintf(stderr, "Allocation failed during perf test\n");
        return 0;
      }
    }
    // Free in reverse order (stress coalescing)
    for (int i = allocs_per_round - 1; i >= 0; i--) {
      my_free(ptrs[i]);
    }
  }
  clock_t end = clock();
  double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
  printf("(Perf: %.3f sec for %d alloc/free cycles)", time_taken,
         rounds * allocs_per_round);
  // Just ensure it runs without crash; performance is observational
  return 1;
}

// -------------------------------
// Main
// -------------------------------
int main(void) {
  // Initialize memory pool (optional, for debugging)
  memset(my_mem_pool, 0, POOL_SIZE);
  my_init();

  printf("=== Memory Allocator Test Suite ===\n");

  RUN_TEST(test_basic);
  RUN_TEST(test_zero_size);
  RUN_TEST(test_max_size);
  RUN_TEST(test_oversize);
  RUN_TEST(test_free_null);
  RUN_TEST(test_double_free);
  RUN_TEST(test_use_after_free);
  RUN_TEST(test_fragmentation);
  RUN_TEST(test_alignment);
  RUN_TEST(test_performance);

  printf("\n=== Summary ===\n");
  printf("Passed: %d\n", test_passed);
  printf("Failed: %d\n", test_failed);
  if (test_failed == 0) {
    printf("All tests passed!\n");
  } else {
    printf("Some tests failed. Check implementation.\n");
  }

  return test_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}