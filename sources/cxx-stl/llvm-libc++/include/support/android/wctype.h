#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_WCTYPES_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_WCTYPES_H

#include_next <wctype.h>
#include <xlocale.h>

extern "C" {

// Add missing declarations from the NDK header. Implemented under
// src/android/wctype.cc

int iswblank(wint_t c);

int iswspace_l(wint_t, locale_t);
int iswprint_l(wint_t, locale_t);
int iswcntrl_l(wint_t, locale_t);
int iswupper_l(wint_t, locale_t);
int iswlower_l(wint_t, locale_t);
int iswalpha_l(wint_t, locale_t);
int iswdigit_l(wint_t, locale_t);
int iswpunct_l(wint_t, locale_t);
int iswxdigit_l(wint_t, locale_t);
int iswblank_l(wint_t, locale_t);

wint_t towlower_l(wint_t, locale_t);
wint_t towupper_l(wint_t, locale_t);

}  // extern "C"

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_WCTYPES_H
