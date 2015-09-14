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
#define __STDC_FORMAT_MACROS 1

#include <cpu-features.h>
#include <inttypes.h>
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
    case ANDROID_CPU_FAMILY_MIPS:
        printf("CPU family is MIPS\n");
        break;
    case ANDROID_CPU_FAMILY_ARM64:
        printf("CPU family is ARM64\n");
        break;
    case ANDROID_CPU_FAMILY_X86_64:
        printf("CPU family is x86_64\n");
        break;
    case ANDROID_CPU_FAMILY_MIPS64:
        printf("CPU family is MIPS64\n");
        break;
    default:
        fprintf(stderr, "Unsupported CPU family: %d\n", family);
        return 1;
    }

#define CHECK(arch, name) \
    if ((features & ANDROID_CPU_## arch ##_FEATURE_## name) != 0) { \
        printf( "  "#name"\n" ); \
    }

    uint64_t features = android_getCpuFeatures();
    int result = 0;
    switch(family) {
    case ANDROID_CPU_FAMILY_ARM:
        printf( "Supported ARM features:\n");
        CHECK(ARM, ARMv7)
        CHECK(ARM, VFPv3)
        CHECK(ARM, NEON)
        CHECK(ARM, LDREX_STREX)
        CHECK(ARM, VFPv2)
        CHECK(ARM, VFP_D32)
        CHECK(ARM, VFP_FP16)
        CHECK(ARM, VFP_FMA)
        CHECK(ARM, NEON_FMA)
        CHECK(ARM, IDIV_ARM)
        CHECK(ARM, IDIV_THUMB2)
        CHECK(ARM, iWMMXt)
        CHECK(ARM, AES)
        CHECK(ARM, PMULL)
        CHECK(ARM, SHA1)
        CHECK(ARM, SHA2)
        CHECK(ARM, CRC32)
        break;
    case ANDROID_CPU_FAMILY_ARM64:
        CHECK(ARM64, FP)
        CHECK(ARM64, ASIMD)
        CHECK(ARM64, AES)
        CHECK(ARM64, PMULL)
        CHECK(ARM64, SHA1)
        CHECK(ARM64, SHA2)
        CHECK(ARM64, CRC32)
        break;
    case ANDROID_CPU_FAMILY_X86:
    case ANDROID_CPU_FAMILY_X86_64:
        printf( "Supported x86 features:\n");
        CHECK(X86, SSSE3)
        CHECK(X86, POPCNT)
        CHECK(X86, MOVBE)
        CHECK(X86, SSE4_1)
        CHECK(X86, SSE4_2)
        break;
    case ANDROID_CPU_FAMILY_MIPS:
    case ANDROID_CPU_FAMILY_MIPS64:
        printf( "Supported MIPS features:\n");
        CHECK(MIPS, R6)
        CHECK(MIPS, MSA)
        break;
    default:
        if (features != 0) {
            printf("ERROR: Unexpected CPU features mask: %016" PRIX64 "\n",
                   features);
            result = 1;
        }
    }

#ifdef __arm__
    uint32_t cpu_id = android_getCpuIdArm();
    printf( "ARM CpuID: %08x\n", cpu_id);
    printf( "   implementer: %02x\n", (cpu_id >> 24) & 0xff);
    printf( "   variant    : %02x\n", (cpu_id >> 20) & 0x0f);
    printf( "   part       : %03x\n", (cpu_id >> 4) & 0xfff);
    printf( "   revision   : %x\n",    cpu_id & 0x0f);
#endif

    int count = android_getCpuCount();
    printf( "Number of CPU cores: %d\n", count);
    return result;
}
