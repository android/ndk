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

#pragma version(1)
#pragma rs java_package_name(com.android.rs.cppbasic)
#pragma rs_fp_relaxed

int g_i = 4;

float g_f = 5.9;

const static float3 gMonoMult = {0.299f, 0.587f, 0.114f};

bool *failed;

#define _RS_ASSERT(b) \
do { \
    if (!(b)) { \
        *failed = true; \
        rsDebug(#b " FAILED", 0); \
    } \
\
} while (0)

struct myStruct {
    int i;
    int j;
    float f;
    char c[3];
};

rs_allocation alloc;
rs_element elem;
rs_type type;
rs_sampler sampler;
rs_script script;

void root(const uchar4 *v_in, uchar4 *v_out) {
    float4 f4 = rsUnpackColor8888(*v_in);

    float3 mono = dot(f4.rgb, gMonoMult);
    *v_out = rsPackColorTo8888(mono);
}

void foo(int i, float f) {
    rsDebug("g_i", g_i);
    rsDebug("g_f", g_f);
    rsDebug("i", i);
    rsDebug("f", f);
}

void bar(int i, int j, char k, int l, int m, int n) {
    _RS_ASSERT(i == 47);
    _RS_ASSERT(j == -3);
    _RS_ASSERT(k == 'c');
    _RS_ASSERT(l == -7);
    _RS_ASSERT(m == 14);
    _RS_ASSERT(n == -8);
}

int __attribute__((kernel)) kern1(int i, uint32_t x, uint32_t y) {
    return i + 10 * x + 100 *y;
}

void __attribute__((kernel)) verify_kern1(int i, uint32_t x, uint32_t y) {
    _RS_ASSERT(i == (5 + 10 * x + 100 * y));
    rsDebug("i ", i);
}

