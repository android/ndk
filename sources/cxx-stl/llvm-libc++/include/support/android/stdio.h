// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_STDIO_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_STDIO_H

// This is to avoid a compiler error when the putc() macro definition
// in <stdio.h> follows a putc() function definition which is apparently
// not compatible with it.
#define _POSIX_THREADS 1
#include_next <stdio.h>

#include <stdarg.h>
#include <xlocale.h>

#ifdef __cplusplus
extern "C" {
#endif

int asprintf_l(char**, locale_t, const char*, ...);
int sprintf_l(char*, locale_t, const char*, ...);
int snprintf_l(char*, size_t, locale_t, const char*, ...);
int sscanf_l(const char*, locale_t, const char*, ...);

int vfwscanf(FILE*, const wchar_t*, va_list);
int vswscanf(const wchar_t *, const wchar_t *, va_list);
int vwscanf(const wchar_t *, va_list);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_STDIO_H
