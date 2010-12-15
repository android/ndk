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

/* This program tests for a compiler bug that was found in the
 * arm-linux-androideabi-4.4.3 toolchain and fixed by the change at
 * https://review.source.android.com/#change,19474
 *
 * The bug generated an invalid sequence of thumb machine instructions
 * when signed chars were being used.
 */
#include <stdio.h>

int test(signed char anim_col)
{
    if (anim_col >= 31) {
        return 1;
    } else if (anim_col <= -15) {
        return -2;
    }
    return 0;
}

int main(void)
{
    const int testval = -7;
    const int expected = 0;
    int ret = test(testval);

    if (ret != expected) {
        fprintf(stderr, "ERROR: test(%d) returned %d instead of %d\n",
                        testval, ret, expected);
        return 1;
    }
    printf("OK: test(%d) returned %d\n", testval, expected);
    return 0;
}
