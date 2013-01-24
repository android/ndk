// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_MATH_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_MATH_H

#include_next <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO(digit): Check that this is not needed for Clang.
typedef double      double_t;
typedef double      float_t;

// Missing long double functions. Note that 'long double' is the same
// than 'double' on Android, so this will define stubs.
#define LLVM_LIBCXX_LONG_DOUBLE_FUNCTIONS
long double     acosl(long double);
long double     asinl(long double);
long double     atanl(long double);
long double     atan2l(long double x, long double y);
long double     cosl(long double);
long double     coshl(long double);
long double     expl(long double);
long double     fmodl(long double, long double);
long double     powl(long double, long double);
long double     sinl(long double);
long double     sinhl(long double);
long double     sqrtl(long double);
long double     tanl(long double);
long double     tanhl(long double);
long double     acoshl(long double);
long double     asinhl(long double);
long double     atanhl(long double);
long double     cbrtl(long double);
long double     erfl(long double);
long double     erfcl(long double);
long double     expm1l(long double);
long double     hypotl(long double, long double);
long double     lgammal(long double);
long long int   llrintl(long double);
long double     logl(long double);
long double     log1pl(long double);
long double     log2l(long double);
long double     logbl(long double);
long double     log10l(long double);
long double     nanl(const char*);
long double     nearbyintl(long double);
long double     remainderl(long double, long double);
long double     remquol(long double, long double, int*);
long double     rintl(long double);
long int        lrintl(long double);
long double     tgammal(long double);
long double     modfl(long double, long double*);
long double     exp2l(long double);

float           tgammaf(float);
double          nan(const char*);
float           nanf(const char*);

float           log2f(float);
double          log2(double);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  /* LLVM_LIBCXX_SUPPORT_ANDROID_MATH_H */
