/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <fenv.h>

#define ASSERT_TRUE(condition) \
  (condition)? (void)0 : fail(__FILE__, __LINE__, __func__, #condition)

#define ASSERT_EQ(x, y) \
  ((x)==(y))? (void)0 : fail(__FILE__, __LINE__, __func__, #x "==" #y)

#define ASSERT_FLOAT_EQ(x, y) \
  float_eq(x, y)? (void)0 : fail(__FILE__, __LINE__, __func__, "float_eq(" #x "," #y ")")

#define TEST(f, g) void g()

int total_fail = 0;
void fail(const char* file, int line, const char* func, const char* expr)
{
    printf("ERROR %s:%d %s: %s\n", file, line, func, expr);
    total_fail++;
}

/* See AOSP external/gtest/include/gtest/internal/gtest-internal.h */
const int kMaxUlps = 4;

int float_eq(float x, float y) {
    int32_t ix0, iy0, ix, iy;

    if (isnanf(x) || isnanf(y))
        return 0;

    ix = ix0 = *(int32_t *)&x;
    iy = iy0 = *(int32_t *)&y;
    if (ix < 0) {
        ix = -ix;
        if (!(iy0 < 0))
            return 0;
    }
    if (iy < 0) {
        iy = -iy;
        if (!(ix0 < 0))
            return 0;
    }
    return abs(ix - iy) <= kMaxUlps;
}

/* See AOSP bionic/tests/fenv_test.cpp */

static void TestRounding(float expectation1, float expectation2) {
  // volatile to prevent compiler optimizations.
  volatile float f = 1.968750f;
  volatile float m = 0x1.0p23f;
  volatile float x = f + m;
  ASSERT_FLOAT_EQ(expectation1, x);
  x -= m;
  ASSERT_EQ(expectation2, x);
}

static void DivideByZero() {
  // volatile to prevent compiler optimizations.
  volatile float zero = 0.0f;
  volatile float result __attribute__((unused)) = 123.0f / zero;
}

TEST(fenv, fesetround_fegetround_FE_TONEAREST) {
  fesetround(FE_TONEAREST);
  ASSERT_EQ(FE_TONEAREST, fegetround());
  TestRounding(8388610.0f, 2.0f);
}

TEST(fenv, fesetround_fegetround_FE_TOWARDZERO) {
  fesetround(FE_TOWARDZERO);
  ASSERT_EQ(FE_TOWARDZERO, fegetround());
  TestRounding(8388609.0f, 1.0f);
}

TEST(fenv, fesetround_fegetround_FE_UPWARD) {
  fesetround(FE_UPWARD);
  ASSERT_EQ(FE_UPWARD, fegetround());
  TestRounding(8388610.0f, 2.0f);
}

TEST(fenv, fesetround_fegetround_FE_DOWNWARD) {
  fesetround(FE_DOWNWARD);
  ASSERT_EQ(FE_DOWNWARD, fegetround());
  TestRounding(8388609.0f, 1.0f);
}

TEST(fenv, feclearexcept_fetestexcept) {
  // Clearing clears.
  feclearexcept(FE_ALL_EXCEPT);
  ASSERT_EQ(0, fetestexcept(FE_ALL_EXCEPT));

  // Dividing by zero sets FE_DIVBYZERO.
  DivideByZero();
  int raised = fetestexcept(FE_DIVBYZERO | FE_OVERFLOW);
  ASSERT_TRUE((raised & FE_OVERFLOW) == 0);
  ASSERT_TRUE((raised & FE_DIVBYZERO) != 0);

  // Clearing an unset bit is a no-op.
  feclearexcept(FE_OVERFLOW);
  ASSERT_TRUE((raised & FE_OVERFLOW) == 0);
  ASSERT_TRUE((raised & FE_DIVBYZERO) != 0);

  // Clearing a set bit works.
  feclearexcept(FE_DIVBYZERO);
  ASSERT_EQ(0, fetestexcept(FE_ALL_EXCEPT));
}

int main()
{
    fesetround_fegetround_FE_TONEAREST();
    fesetround_fegetround_FE_TOWARDZERO();
    fesetround_fegetround_FE_UPWARD();
    fesetround_fegetround_FE_DOWNWARD();
    feclearexcept_fetestexcept();
    printf("total_fail = %d\n", total_fail);
    return total_fail == 0 ? 0 : 1;
}
