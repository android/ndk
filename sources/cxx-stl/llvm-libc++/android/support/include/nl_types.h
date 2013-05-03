// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_NL_TYPES_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_NL_TYPES_H

#define NL_SETD 1
#define NL_CAT_LOCALE 1

#ifdef __cplusplus
extern "C" {
#endif

typedef void* nl_catd;
typedef int nl_item;

nl_catd  catopen(const char*, int);
char*    catgets(nl_catd, int, int, const char*);
int      catclose(nl_catd);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  /* LLVM_LIBCXX_SUPPORT_ANDROID_NL_TYPES_H */

