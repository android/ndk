#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_TIME_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_TIME_H

#include_next <time.h>
#include <xlocale.h>

extern "C" {

size_t strftime_l(char *s, size_t maxsize, const char *format,
                  const struct tm * timeptr, locale_t locale);

}  // extern "C"

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_TIME_H
