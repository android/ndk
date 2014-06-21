/*
 * Copyright (C) 2010 The Android Open Source Project
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
#include <android/api-level.h>
#include <features.h>
#include <math.h>
#include <stdio.h>

long double ld, ld1, ldn;
double d, d1, dn;
float f, f1, fn;
int fail_count = 0;

void TEST_EQ(long double a, long double b)
{
    long double diff = a - b;
    if (diff < 0)
       diff = -diff;
    if (diff > 0.0000001)
       fail_count ++;
}

int main()
{
#if __ANDROID_API__ >= 9
#if  __ANDROID_API__ < 20
    sincosl(0.73, &ld, &ld1);
    sincos(0.72, &d, &d1);
    sincosf(0.71f, &f, &f1);
    TEST_EQ(ld, 0.6668696L);
    TEST_EQ(ld1, 0.7451744L);
    TEST_EQ(d, 0.6593847);
    TEST_EQ(d1, 0.7518057);
    TEST_EQ(f, 0.6518338f);
    TEST_EQ(f1, 0.7583619f);
#endif
#if __ANDROID_API__ >= 13
    ldn = nanl("");
    dn = nan("");
    fn = nanf("");
#if __ANDROID_API__ >= 18
    ld = 1234L;
    ld = logbl(ld);
    TEST_EQ(ld, 10L);
    ld = log2l(ld);
    TEST_EQ(ld, 3.321928L);
    d  = 56.78;
    d = log2(d);
    TEST_EQ(d, 5.827311);
    f  = 0.9012f;
    f = log2f(f);
    TEST_EQ(f, -.1500808f);
    ld = nexttowardl(ld, 0);
    d = nexttoward(d, 0);
#endif
#endif
#endif
    if (fail_count)
        printf("fail_count = %d\n", fail_count);

    return fail_count;
}
