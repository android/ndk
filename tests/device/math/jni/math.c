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

long double ld, ld1, ldn;
double d, d1, dn;
float f, f1, fn;

int main()
{
#if __ANDROID_API__ >= 9
    sincosl(0.73, &ld, &ld1);
    sincos(0.72, &d, &d1);
    sincosf(0.71f, &f, &f1);
#if __ANDROID_API__ >= 13
    ldn = nanl("");
    dn = nan("");
    fn = nanf("");
#if __ANDROID_API__ >= 18
    ld = logbl(ld);
    ld = log2l(ld);
    d = log2(d);
    f = log2f(f);
    ld = nexttowardl(ld, 0);
    d = nexttoward(d, 0);
#endif
#endif
#endif
    return 0;
}
