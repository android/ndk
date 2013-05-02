// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct locale_struct* locale_t;

struct locale_struct {
    void* dummy;
};

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_XLOCALE_H
