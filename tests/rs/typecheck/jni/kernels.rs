/*
 * Copyright (C) 2013 The Android Open Source Project
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
#pragma rs java_package_name(com.android.rs.typecheck)
#pragma rs_fp_relaxed

// Test initialized and uninitialized variables
char c1;
char c1i = 1;
char2 c2;
char2 c2i = {1, 2};
char3 c3;
char3 c3i = {1, 2, 3};
char4 c4;
char4 c4i = {1, 2, 3, 4};

uchar uc1;
uchar uc1i = 1;
uchar2 uc2;
uchar2 uc2i = {1, 2};
uchar3 uc3;
uchar3 uc3i = {1, 2, 3};
uchar4 uc4;
uchar4 uc4i = {1, 2, 3, 4};

short s1;
short s1i = 1;
short2 s2;
short2 s2i = {1, 2};
short3 s3;
short3 s3i = {1, 2, 3};
short4 s4;
short4 s4i = {1, 2, 3, 4};

ushort us1;
ushort us1i = 1;
ushort2 us2;
ushort2 us2i = {1, 2};
ushort3 us3;
ushort3 us3i = {1, 2, 3};
ushort4 us4;
ushort4 us4i = {1, 2, 3, 4};

int i1;
int i1i = 1;
int2 i2;
int2 i2i = {1, 2};
int3 i3;
int3 i3i = {1, 2, 3};
int4 i4;
int4 i4i = {1, 2, 3, 4};

uint ui1;
uint ui1i = 1;
uint2 ui2;
uint2 ui2i = {1, 2};
uint3 ui3;
uint3 ui3i = {1, 2, 3};
uint4 ui4;
uint4 ui4i = {1, 2, 3, 4};

long l1;
long l1i = 1;
long2 l2;
long2 l2i = {1, 2};
long3 l3;
long3 l3i = {1, 2, 3};
long4 l4;
long4 l4i = {1, 2, 3, 4};

ulong ul1;
ulong ul1i = 1;
ulong2 ul2;
ulong2 ul2i = {1, 2};
ulong3 ul3;
ulong3 ul3i = {1, 2, 3};
ulong4 ul4;
ulong4 ul4i = {1, 2, 3, 4};

float f1;
float f1i = 3.141592265358979f;
float2 f2;
float2 f2i = {1.f, 2.f};
float3 f3;
float3 f3i = {1.f, 2.f, 3.f};
float4 f4;
float4 f4i = {1.f, 2.f, 3.f, 4.f};

double d1;
double d1i = 3.141592265358979;
double2 d2;
double2 d2i = {1, 2};
double3 d3;
double3 d3i = {1, 2, 3};
double4 d4;
double4 d4i = {1, 2, 3, 4};


void __attribute__((kernel)) test_BOOLEAN(bool in) {
}

void __attribute__((kernel)) test_I8(char in) {
}

void __attribute__((kernel)) test_U8(uchar in) {
}

void __attribute__((kernel)) test_I16(short in) {
}

void __attribute__((kernel)) test_U16(ushort in) {
}

void __attribute__((kernel)) test_I32(int in) {
}

void __attribute__((kernel)) test_U32(uint in) {
}

void __attribute__((kernel)) test_I64(long in) {
}

void __attribute__((kernel)) test_U64(ulong in) {
}

void __attribute__((kernel)) test_F32(float in) {
}

void __attribute__((kernel)) test_F64(double in) {
}

