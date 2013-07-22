// This checks that simply including <wchar.h> with
// _WCHAR_IS_8BIT defined will provice an 8-bit wchar_t
// and 8-bit WCHAR_MIN/WCHAR_MAX.
#include <android/api-level.h>

// Force WCHAR_MIN/WCHAR_MAX to 32-bit values.
#define __STDC_LIMIT_MACROS
// Force wchar_t to be 8 bits.
#define _WCHAR_IS_8BIT
#include <wchar.h>

#if defined(__arm__) && __ANDROID_API__ != 3
#error "You should target API level 3 when compiling this file!"
#endif

#define CONCAT(x,y) CONCAT_(x,y)
#define CONCAT_(x,y) x ## y

#define STATIC_ASSERT(condition) \
  static char CONCAT(dummy_,__LINE__)[1 - 2*(!(condition))];

#ifdef __arm__
STATIC_ASSERT(sizeof(__WCHAR_TYPE__) == 1);
#else
STATIC_ASSERT(sizeof(__WCHAR_TYPE__) == 4);
#endif

// wchar_t is never redefined by <stddef.h> because it's a C++ keyword,
// unlike in C.
STATIC_ASSERT(sizeof(wchar_t) == 4);

// This is C++ code but __STDC_LIMIT_MACROS was defined, and
// _WCHAR_IS_8BIT is defined, so the values are always 32-bit signed.
STATIC_ASSERT(WCHAR_MIN == 0x80000000);
STATIC_ASSERT(WCHAR_MAX == 0x7fffffff);
