// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_TIME_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_TIME_H

#include_next <time.h>
#include <xlocale.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t strftime_l(char *s, size_t maxsize, const char *format,
                  const struct tm * timeptr, locale_t locale);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_TIME_H
