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
    case ANDROID_CPU_FAMILY_MIPS:
        printf("CPU family is MIPS\n");
        break;
    default:
        fprintf(stderr, "Unsupported CPU family: %d\n", family);
        return 1;
    }

    if (family == ANDROID_CPU_FAMILY_ARM) {
        uint64_t features = android_getCpuFeatures();
        printf( "Supported ARM features:\n");
#define CHECK(name) \
        if ((features & ANDROID_CPU_ARM_FEATURE_## name) != 0) { \
            printf( "  "#name"\n" ); \
        }
        CHECK(LDREX_STREX)
        CHECK(VFPv2)
        CHECK(ARMv7)
        CHECK(VFPv3)
        CHECK(VFP_D32)
        CHECK(VFP_FP16)
        CHECK(VFP_FMA)
        CHECK(NEON)
        CHECK(NEON_FMA)
        CHECK(IDIV_ARM)
        CHECK(IDIV_THUMB2)
        CHECK(iWMMXt)
#undef CHECK
    }

#ifdef __arm__
    uint32_t cpu_id = android_getCpuIdArm();
    printf( "ARM CpuID: %08x\n", cpu_id);
    printf( "   implementer: %02x\n", (cpu_id >> 24) & 0xff);
    printf( "   variant    : %02x\n", (cpu_id >> 20) & 0x0f);
    printf( "   part       : %03x\n", (cpu_id >> 4) & 0xfff);
    printf( "   revision   : %x\n",    cpu_id & 0x0f);
#endif

    if (family == ANDROID_CPU_FAMILY_X86) {
        uint64_t features = android_getCpuFeatures();
        printf( "Supported x86 features:\n");
#define CHECK(name) \
        if ((features & ANDROID_CPU_X86_FEATURE_## name) != 0) { \
            printf( "  "#name"\n" ); \
        }
        CHECK(SSSE3)
        CHECK(POPCNT)
        CHECK(MOVBE)
#undef CHECK
    }

    int count = android_getCpuCount();
    printf( "Number of CPU cores: %d\n", count);
    return 0;
}
