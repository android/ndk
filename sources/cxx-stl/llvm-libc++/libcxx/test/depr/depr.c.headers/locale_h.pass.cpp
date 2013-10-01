//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <locale.h>

#include <locale.h>
#include <type_traits>

#if defined(__ANDROID__)
// In Android the following are not defined as symbol but instead in enum
int lc_all = LC_ALL;
int lc_collate = LC_COLLATE;
int lc_ctype = LC_CTYPE;
int lc_monetary = LC_MONETARY;
int lc_numeric = LC_NUMERIC;
int lc_time = LC_TIME;

#else

#ifndef LC_ALL
#error LC_ALL not defined
#endif

#ifndef LC_COLLATE
#error LC_COLLATE not defined
#endif

#ifndef LC_CTYPE
#error LC_CTYPE not defined
#endif

#ifndef LC_MONETARY
#error LC_MONETARY not defined
#endif

#ifndef LC_NUMERIC
#error LC_NUMERIC not defined
#endif

#ifndef LC_TIME
#error LC_TIME not defined
#endif

#endif // __ANDROID__

#ifndef NULL
#error NULL not defined
#endif

int main()
{
    lconv lc;
    static_assert((std::is_same<__typeof__(setlocale(0, "")), char*>::value), "");
    static_assert((std::is_same<__typeof__(localeconv()), lconv*>::value), "");
}
