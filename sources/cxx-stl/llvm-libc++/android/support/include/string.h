// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_STRING_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_STRING_H

#include_next <string.h>
#include <xlocale.h>

#ifdef __cplusplus
extern "C" {
#endif

int strcoll_l(const char*, const char*, locale_t);
int strxfrm_l(char*, const char*, size_t, locale_t);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_STRING_H
