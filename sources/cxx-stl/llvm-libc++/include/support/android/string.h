#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_STRING_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_STRING_H

#include_next <string.h>
#include <xlocale.h>

extern "C" {

int strcoll_l(const char*, const char*, locale_t);
int strxfrm_l(char*, const char*, size_t, locale_t);

}  // extern "C"

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_STRING_H
