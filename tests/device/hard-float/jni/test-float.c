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

int main()
{
    double d = foo(d1,d2);
    const double expected = 4.483853;
    printf("%lf\n", d);
    return (fabs(d-expected) < 0.00001)? 0 : 1;
}
