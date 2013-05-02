// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_STDLIB_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_STDLIB_H

#include_next <stdlib.h>
#include <xlocale.h>

#ifdef __cplusplus
extern "C" {
#endif

long long   strtoll(const char*, char**, int);
long double strtold(const char*, char**);
void _Exit(int);

long                 strtol_l(const char *nptr, char **endptr, int base, locale_t loc);
long long            strtoll_l(const char *nptr, char **endptr, int base, locale_t loc);
unsigned long        strtoul_l(const char *nptr, char **endptr, int base, locale_t loc);
unsigned long long   strtoull_l(const char *nptr, char **endptr, int base, locale_t loc);
long double          strtold_l (const char *nptr, char **endptr, locale_t loc);

int                  mbtowc(wchar_t *pwc, const char *pmb, size_t max);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_STDLIB_H
