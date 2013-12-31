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

#include <math.h>
#include <stdio.h>

double foo(double a, double b)
{
   return sin(a) + cos(b) + 4.9;
}

double d1 = M_PI;
double d2 = 2.0;
double d3 = -0.007;
double d4 = 0.5;


int main()
{
    int fail_count = 0;
    {
        double d0 = foo(d1,d2);
        const double expected0 = 4.483853;
        printf("%lf\n", d0);
        fail_count += !(fabs(d0-expected0) < 0.00001);
    }
    {
        double d1 = __builtin_atan2(d3, d4);
        const double expected1 = -0.013999;
        printf("%lf\n", d1);
        fail_count += !(fabs(d1-expected1) < 0.00001);
    }
    return fail_count;
}
