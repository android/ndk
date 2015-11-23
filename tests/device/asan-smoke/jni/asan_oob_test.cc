//===-- asan_oob_test.cc --------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
//===----------------------------------------------------------------------===//
#include <stdio.h>

#include <gtest/gtest.h>

#define NOINLINE __attribute__((noinline))

typedef uint8_t   U1;
typedef uint16_t  U2;
typedef uint32_t  U4;
typedef uint64_t  U8;

const size_t kLargeMalloc = 1 << 24;

// Make the compiler thinks that something is going on there.
inline void break_optimization(void *arg) {
#if !defined(_WIN32) || defined(__clang__)
  __asm__ __volatile__("" : : "r" (arg) : "memory");
#endif
}

NOINLINE void *malloc_fff(size_t size) {
  void *res = malloc/**/(size); break_optimization(0); return res;}
NOINLINE void *malloc_eee(size_t size) {
  void *res = malloc_fff(size); break_optimization(0); return res;}
NOINLINE void *malloc_ddd(size_t size) {
  void *res = malloc_eee(size); break_optimization(0); return res;}
NOINLINE void *malloc_ccc(size_t size) {
  void *res = malloc_ddd(size); break_optimization(0); return res;}
NOINLINE void *malloc_bbb(size_t size) {
  void *res = malloc_ccc(size); break_optimization(0); return res;}
NOINLINE void *malloc_aaa(size_t size) {
  void *res = malloc_bbb(size); break_optimization(0); return res;}

NOINLINE void free_ccc(void *p) { free(p); break_optimization(0);}
NOINLINE void free_bbb(void *p) { free_ccc(p); break_optimization(0);}
NOINLINE void free_aaa(void *p) { free_bbb(p); break_optimization(0);}

template<typename T>
NOINLINE void asan_write(T *a) {
  *a = 0;
}

template<typename T>
NOINLINE void oob_test(int size, int off) {
  char *p = (char*)malloc_aaa(size);
  // fprintf(stderr, "writing %d byte(s) into [%p,%p) with offset %d\n",
  //        sizeof(T), p, p + size, off);
  asan_write((T*)(p + off));
  free_aaa(p);
}

template<typename T>
void OOBTest() {
  char expected_str[100];
  for (int size = sizeof(T); size < 20; size += 5) {
    for (int i = -5; i < 0; i++) {
      const char *str =
          "is located.*%d byte.*to the left";
      sprintf(expected_str, str, abs(i));
      EXPECT_DEATH(oob_test<T>(size, i), expected_str);
    }

    for (int i = 0; i < (int)(size - sizeof(T) + 1); i++)
      oob_test<T>(size, i);

    for (int i = size - sizeof(T) + 1; i <= (int)(size + 2 * sizeof(T)); i++) {
      const char *str =
          "is located.*%d byte.*to the right";
      int off = i >= size ? (i - size) : 0;
      // we don't catch unaligned partially OOB accesses.
      if (i % sizeof(T)) continue;
      sprintf(expected_str, str, off);
      EXPECT_DEATH(oob_test<T>(size, i), expected_str);
    }
  }

  EXPECT_DEATH(oob_test<T>(kLargeMalloc, -1),
          "is located.*1 byte.*to the left");
  EXPECT_DEATH(oob_test<T>(kLargeMalloc, kLargeMalloc),
          "is located.*0 byte.*to the right");
}

TEST(AddressSanitizer, OOB_char) {
  OOBTest<U1>();
}

TEST(AddressSanitizer, OOB_int) {
  OOBTest<U4>();
}
