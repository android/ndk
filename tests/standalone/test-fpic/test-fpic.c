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

/* This test is used to check that -fpic is a default compiler option
 * for the arm-linux-androideabi toolchain.
 */
#ifdef __arm__
#ifndef __PIC__
#error __PIC__ is not defined!
#endif
#if __PIC__ == 2
#error -fPIC is the default, should be -fpic
#endif
#if __PIC__ != 1
#error __PIC__ value is unsupported! Should be 1 to indicate -fpic is the default.
#endif
#endif

int main(void)
{
    return 0;
}
