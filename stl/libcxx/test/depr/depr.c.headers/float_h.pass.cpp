//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

 // test <float.h>

#include <float.h>

#ifndef FLT_ROUNDS
#error FLT_ROUNDS not defined
#endif

#ifndef FLT_EVAL_METHOD
#if !defined(__clang__) && !defined(__FLT_EVAL_METHOD__)
// GCC defines __FLT_EVAL_METHOD__ in lib/gcc/arm-linux-androideabi/4.8/include/float.h.
// In libc++ include/cfloat define FLT_EVAL_METHOD to __FLT_EVAL_METHOD__
#error FLT_EVAL_METHOD not defined
#endif
#endif

#ifndef FLT_RADIX
#error FLT_RADIX not defined
#endif

#ifndef FLT_MANT_DIG
#error FLT_MANT_DIG not defined
#endif

#ifndef DBL_MANT_DIG
#error DBL_MANT_DIG not defined
#endif

#ifndef LDBL_MANT_DIG
#error LDBL_MANT_DIG not defined
#endif

#ifndef DECIMAL_DIG
#if !defined(__clang__) && !defined(__DECIMAL_DIG__)
// GCC defines __DECIMAL_DIG__ in lib/gcc/arm-linux-androideabi/4.8/include/float.h.
// In libc++ include/cfloat define DECIMAL_DIG to __DECIMAL_DIG__
#error DECIMAL_DIG not defined
#endif
#endif

#ifndef FLT_DIG
#error FLT_DIG not defined
#endif

#ifndef DBL_DIG
#error DBL_DIG not defined
#endif

#ifndef LDBL_DIG
#error LDBL_DIG not defined
#endif

#ifndef FLT_MIN_EXP
#error FLT_MIN_EXP not defined
#endif

#ifndef DBL_MIN_EXP
#error DBL_MIN_EXP not defined
#endif

#ifndef LDBL_MIN_EXP
#error LDBL_MIN_EXP not defined
#endif

#ifndef FLT_MIN_10_EXP
#error FLT_MIN_10_EXP not defined
#endif

#ifndef DBL_MIN_10_EXP
#error DBL_MIN_10_EXP not defined
#endif

#ifndef LDBL_MIN_10_EXP
#error LDBL_MIN_10_EXP not defined
#endif

#ifndef FLT_MAX_EXP
#error FLT_MAX_EXP not defined
#endif

#ifndef DBL_MAX_EXP
#error DBL_MAX_EXP not defined
#endif

#ifndef LDBL_MAX_EXP
#error LDBL_MAX_EXP not defined
#endif

#ifndef FLT_MAX_10_EXP
#error FLT_MAX_10_EXP not defined
#endif

#ifndef DBL_MAX_10_EXP
#error DBL_MAX_10_EXP not defined
#endif

#ifndef LDBL_MAX_10_EXP
#error LDBL_MAX_10_EXP not defined
#endif

#ifndef FLT_MAX
#error FLT_MAX not defined
#endif

#ifndef DBL_MAX
#error DBL_MAX not defined
#endif

#ifndef LDBL_MAX
#error LDBL_MAX not defined
#endif

#ifndef FLT_EPSILON
#error FLT_EPSILON not defined
#endif

#ifndef DBL_EPSILON
#error DBL_EPSILON not defined
#endif

#ifndef LDBL_EPSILON
#error LDBL_EPSILON not defined
#endif

#ifndef FLT_MIN
#error FLT_MIN not defined
#endif

#ifndef DBL_MIN
#error DBL_MIN not defined
#endif

#ifndef LDBL_MIN
#error LDBL_MIN not defined
#endif

int main()
{
}
