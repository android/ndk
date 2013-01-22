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
