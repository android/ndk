// This checks that, by default, wchar_t is 32-bit and
// WCHAR_MIN/WCHAR_MAX are 32-bit signed.
#include <android/api-level.h>
#include <wchar.h>

#if __ANDROID_API__ != 3
#error "You should target API level 3 when compiling this file!"
#endif

#define CONCAT(x,y) CONCAT_(x,y)
#define CONCAT_(x,y) x ## y

#define STATIC_ASSERT(condition) \
  static char CONCAT(dummy_,__LINE__)[1 - 2*(!(condition))];

STATIC_ASSERT(sizeof(wchar_t) == 4);
STATIC_ASSERT(WCHAR_MIN == -1-2147483647);
STATIC_ASSERT(WCHAR_MAX == 2147483647);
