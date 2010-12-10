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
#include <cpu-features.h>
#include <stdio.h>

int main(void)
{
    AndroidCpuFamily family = android_getCpuFamily();
    switch (family) {
    case ANDROID_CPU_FAMILY_ARM:
        printf("CPU family is ARM\n");
        break;
    case ANDROID_CPU_FAMILY_X86:
        printf("CPU family is x86\n");
        break;
    default:
        fprintf(stderr, "Unsupported CPU family: %d\n", family);
        return 1;
    }

    if (family == ANDROID_CPU_FAMILY_ARM) {
        uint64_t features = android_getCpuFeatures();
        printf( "Supported ARM features:\n");
        if ((features & ANDROID_CPU_ARM_FEATURE_ARMv7) != 0) {
            printf( "  ARMv7\n" );
        }
        if ((features & ANDROID_CPU_ARM_FEATURE_VFPv3) != 0) {
            printf( "  VFPv3\n" );
        }
        if ((features & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
            printf( "  NEON\n" );
        }
    }
    return 0;
}
