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

#include "RenderScript.h"
#include "ScriptC_kernels.h"

using namespace android;
using namespace RSC;

const static uint32_t x = 7;

sp<Allocation> createAlloc(sp<RS> rs, sp<const Element> e) {
    Type::Builder tb(rs, e);
    tb.setX(x);
    sp<const Type> t = tb.create();
    return Allocation::createTyped(rs, t);
}

#define TEST_ELEM(KERNELNAME, ENAME) \
bool test_elem_##KERNELNAME##_##ENAME() { \
    printf("Verifying forEach_test_" #KERNELNAME "() with " #ENAME "\n"); \
    sp<RS> rs = new RS(); \
    bool r = rs->init("/system/bin"); \
    sp<Allocation> a = createAlloc(rs, Element::ENAME(rs)); \
    ScriptC_kernels sc(rs); \
    sc.forEach_test_##KERNELNAME(a); \
    rs->finish(); \
    bool shouldPass = !strcmp(#KERNELNAME, #ENAME); \
    if (shouldPass != (rs->getError() == RS_SUCCESS)) { \
        printf("Failed forEach_test_" #KERNELNAME "() with " #ENAME "\n"); \
        return true; \
    } \
    return false; \
}

#define TEST_ELEM_ALL(ENAME) \
TEST_ELEM(ENAME, BOOLEAN) \
TEST_ELEM(ENAME, I8) \
TEST_ELEM(ENAME, U8) \
TEST_ELEM(ENAME, I16) \
TEST_ELEM(ENAME, U16) \
TEST_ELEM(ENAME, I32) \
TEST_ELEM(ENAME, U32) \
TEST_ELEM(ENAME, I64) \
TEST_ELEM(ENAME, U64) \
TEST_ELEM(ENAME, F32) \
TEST_ELEM(ENAME, F64)

TEST_ELEM_ALL(BOOLEAN)
TEST_ELEM_ALL(I8)
TEST_ELEM_ALL(U8)
TEST_ELEM_ALL(I16)
TEST_ELEM_ALL(U16)
TEST_ELEM_ALL(I32)
TEST_ELEM_ALL(U32)
TEST_ELEM_ALL(I64)
TEST_ELEM_ALL(U64)
TEST_ELEM_ALL(F32)
TEST_ELEM_ALL(F64)

int main()
{
    bool failed = false;

#define EXECUTE_TEST_ELEM_ALL(ENAME) \
    failed |= test_elem_##ENAME##_BOOLEAN(); \
    failed |= test_elem_##ENAME##_I8(); \
    failed |= test_elem_##ENAME##_U8(); \
    failed |= test_elem_##ENAME##_I16(); \
    failed |= test_elem_##ENAME##_U16(); \
    failed |= test_elem_##ENAME##_I32(); \
    failed |= test_elem_##ENAME##_U32(); \
    failed |= test_elem_##ENAME##_I64(); \
    failed |= test_elem_##ENAME##_U64(); \
    failed |= test_elem_##ENAME##_F32(); \
    failed |= test_elem_##ENAME##_F64(); \

    EXECUTE_TEST_ELEM_ALL(BOOLEAN);
    EXECUTE_TEST_ELEM_ALL(I8);
    EXECUTE_TEST_ELEM_ALL(U8);
    EXECUTE_TEST_ELEM_ALL(I16);
    EXECUTE_TEST_ELEM_ALL(U16);
    EXECUTE_TEST_ELEM_ALL(I32);
    EXECUTE_TEST_ELEM_ALL(U32);
    EXECUTE_TEST_ELEM_ALL(I64);
    EXECUTE_TEST_ELEM_ALL(U64);
    EXECUTE_TEST_ELEM_ALL(F32);
    EXECUTE_TEST_ELEM_ALL(F64);

    if (failed) {
        printf("TEST FAILED!\n");
    } else {
        printf("TEST PASSED!\n");
    }

    return failed;
}
