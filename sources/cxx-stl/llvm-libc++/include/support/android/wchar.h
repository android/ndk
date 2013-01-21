#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_WCHAR_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_WCHAR_H

#include_next <wchar.h>
#include <xlocale.h>

extern "C" {

// Add missing declarations that are not in the NDK.
float               wcstof(const wchar_t*, wchar_t**);
long                wcstol(const wchar_t* nptr, wchar_t**, int base);
long double         wcstold(const wchar_t*, wchar_t**);
long long           wcstoll(const wchar_t*, wchar_t**, int base);
unsigned long long  wcstoull(const wchar_t*, wchar_t**, int base);

extern size_t wcsnrtombs (char *dst,
                          const wchar_t **src,
                          size_t nwc, size_t len,
                          mbstate_t *ps);

extern size_t mbsnrtowcs (wchar_t *dst,
                          const char **src, size_t nmc,
                          size_t len, mbstate_t *ps);

int wcscoll_l(const wchar_t*, const wchar_t*, locale_t);
int wcsxfrm_l(wchar_t*, const wchar_t*, size_t, locale_t);

}  // extern "C"

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_WCHAR_H
