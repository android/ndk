#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_STDLIB_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_STDLIB_H

#include_next <stdlib.h>
#include <xlocale.h>

extern "C" {

long long   strtoll(const char*, char**, int);
long double strtold(const char*, char**);
void _Exit(int);

long                 strtol_l(const char *nptr, char **endptr, int base, locale_t loc);
long long            strtoll_l(const char *nptr, char **endptr, int base, locale_t loc);
unsigned long        strtoul_l(const char *nptr, char **endptr, int base, locale_t loc);
unsigned long long   strtoull_l(const char *nptr, char **endptr, int base, locale_t loc);
long double          strtold_l (const char *nptr, char **endptr, locale_t loc);

}  // extern "C"

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_STDLIB_H
