// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_CTYPE_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_CTYPE_H

#include_next <ctype.h>
#include <xlocale.h>

#ifdef __cplusplus
extern "C" {
#endif

# define __exctype_l(name)  extern int name (int, locale_t)

__exctype_l (isalnum_l);
__exctype_l (isalpha_l);
__exctype_l (iscntrl_l);
__exctype_l (isdigit_l);
__exctype_l (islower_l);
__exctype_l (isgraph_l);
__exctype_l (isprint_l);
__exctype_l (ispunct_l);
__exctype_l (isspace_l);
__exctype_l (isupper_l);
__exctype_l (isxdigit_l);
__exctype_l (isblank_l);

int tolower_l(int c, locale_t);
int toupper_l(int c, locale_t);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_CTYPE_H
