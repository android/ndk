#include <android/api-level.h>

#if !__LP64__ && !defined(__arm__) || __ANDROID_API__ == 3

// This checks that simply including <wchar.h> with
// _WCHAR_IS_8BIT defined will provice an 8-bit wchar_t
// and 8-bit WCHAR_MIN/WCHAR_MAX.

// Force wchar_t to be 8 bits.
#define _WCHAR_IS_8BIT
#include <wchar.h>

#define CONCAT(x,y) CONCAT_(x,y)
#define CONCAT_(x,y) x ## y

#define STATIC_ASSERT(condition) \
  static char CONCAT(dummy_,__LINE__)[1 - 2*(!(condition))];

#if defined(__arm__) || __ANDROID_API__ < 9
STATIC_ASSERT(sizeof(__WCHAR_TYPE__) == 1);
#else
STATIC_ASSERT(sizeof(__WCHAR_TYPE__) == 4);
#endif

// wchar_t is never redefined by <stddef.h> because it's a C++ keyword,
// unlike in C.
STATIC_ASSERT(sizeof(wchar_t) == 4);

// Since this is C++ code, and __STC_LIMIT_MACROS was not defined, the
// old behaviour on ARM was to define these constants to 8-bit values.
// Otherwise, always signed 32-bit values.
#ifdef __arm__
STATIC_ASSERT(WCHAR_MIN == 0);
STATIC_ASSERT(WCHAR_MAX == 255);
#else
STATIC_ASSERT(WCHAR_MIN == 0x80000000);
STATIC_ASSERT(WCHAR_MAX == 0x7fffffff);
#endif

#endif