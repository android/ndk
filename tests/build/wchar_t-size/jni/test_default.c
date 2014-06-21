#include <android/api-level.h>

#if !__LP64__ && !defined(__arm__) || __ANDROID_API__ == 3

// This checks that, by default, wchar_t is 32-bit and
// WCHAR_MIN/WCHAR_MAX are 32-bit signed on all platforms except ARM.
#include <wchar.h>

#define CONCAT(x,y) CONCAT_(x,y)
#define CONCAT_(x,y) x ## y

#define STATIC_ASSERT(condition) \
  static char CONCAT(dummy_,__LINE__)[1 - 2*(!(condition))];

STATIC_ASSERT(sizeof(wchar_t) == 4);

#if defined(__arm__)
STATIC_ASSERT(WCHAR_MIN == 0U);
STATIC_ASSERT(WCHAR_MAX == 2*2147483647U + 1U);
#else
STATIC_ASSERT(WCHAR_MIN == -1-2147483647);
STATIC_ASSERT(WCHAR_MAX == 2147483647);
#endif

#endif