// -*- C++ -*-
//===-------------------- support/android/locale_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wctype.h>

// Contains an implementation of all locale-specific functions (those
// ending in _l, like strcoll_l()), as simple wrapper to the non-locale
// specific ones for now.
//
// That's because Android's C library doesn't support locales. Or more
// specifically, only supports the "C" one.
//
// TODO(digit): Write a more complete implementation that uses JNI to
//              invoke the platform APIs to implement proper handling.
//

///////////////////////////////////////////////////////////////////////
// ctype.h declarations

# define define_char_wrapper_l(name)  \
  int name ## _l (int ch, locale_t loc) { \
    return name (ch); \
  }

define_char_wrapper_l (isalnum);
define_char_wrapper_l (isalpha);
define_char_wrapper_l (iscntrl);
define_char_wrapper_l (isdigit);
define_char_wrapper_l (islower);
define_char_wrapper_l (isgraph);
define_char_wrapper_l (isprint);
define_char_wrapper_l (ispunct);
define_char_wrapper_l (isspace);
define_char_wrapper_l (isupper);
define_char_wrapper_l (isxdigit);
define_char_wrapper_l (isblank);
define_char_wrapper_l (tolower)
define_char_wrapper_l (toupper)

// TODO(ajwong): This table is copied from bionic's ctype implementation.
// It doesn't support signed chars and will index out of bounds. The best way
// to fix is to patch bionic's ctype array to support both signed and
// unsigned char and then just directly reference it.
static char const real_ctype_c_mask_table[256] = {
        0,
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
        _C,     _C|_S,  _C|_S,  _C|_S,  _C|_S,  _C|_S,  _C,     _C,
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C,
   _S|(char)_B, _P,     _P,     _P,     _P,     _P,     _P,     _P,
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P,
        _N,     _N,     _N,     _N,     _N,     _N,     _N,     _N,
        _N,     _N,     _P,     _P,     _P,     _P,     _P,     _P,
        _P,     _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U|_X,  _U,
        _U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
        _U,     _U,     _U,     _U,     _U,     _U,     _U,     _U,
        _U,     _U,     _U,     _P,     _P,     _P,     _P,     _P,
        _P,     _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L|_X,  _L,
        _L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
        _L,     _L,     _L,     _L,     _L,     _L,     _L,     _L,
        /* determine printability based on the IS0 8859 8-bit standard */
        _L,     _L,     _L,     _P,     _P,     _P,     _P,     _C,

        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C, /* 80 */
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C, /* 88 */
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C, /* 90 */
        _C,     _C,     _C,     _C,     _C,     _C,     _C,     _C, /* 98 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* A0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* A8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* B0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* B8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* C0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* C8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* D0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* D8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* E0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* E8 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P, /* F0 */
        _P,     _P,     _P,     _P,     _P,     _P,     _P,     _P  /* F8 */
};
char const* const __ctype_c_mask_table = &real_ctype_c_mask_table[0];

///////////////////////////////////////////////////////////////////////
// stdio.h declarations

int vasprintf_l(char** strp, locale_t l, const char* fmt, va_list args) {
    // Ignore locale.
    return vasprintf(strp, fmt, args);
}

int asprintf_l(char** strp, locale_t locale, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vasprintf_l(strp, locale, fmt, args);
    va_end(args);
    return result;
}

int vsprintf_l(char* str, locale_t l, const char* fmt, va_list args) {
    // Ignore locale.
    return vsprintf(str, fmt, args);
}

int sprintf_l(char* str, locale_t l, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vsprintf_l(str, l, fmt, args);
    va_end(args);
    return result;
}

int vsnprintf_l(char* str, size_t size, locale_t l, const char* fmt, va_list args) {
    return vsnprintf(str, size, fmt, args);
}

int snprintf_l(char* str, size_t size, locale_t l, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int result = vsnprintf_l(str, size, l, fmt, args);
  va_end(args);
  return result;
}

int vsscanf_l(const char* str, locale_t l, const char* fmt, va_list args) {
    return vsscanf(str, fmt, args);
}

int sscanf_l(const char* str, locale_t l, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vsscanf_l(str, l, fmt, args);
    va_end(args);
    return result;
}

///////////////////////////////////////////////////////////////////////
// stdlib.h declarations

long strtol_l(const char *nptr, char **endptr, int base, locale_t loc) {
    return strtol(nptr, endptr, base);
}

long long strtoll_l(const char *nptr, char **endptr, int base, locale_t loc) {
    return strtoll(nptr, endptr, base);
}

unsigned long strtoul_l(const char *nptr, char **endptr, int base, locale_t loc) {
    return strtoul(nptr, endptr, base);
}

unsigned long long strtoull_l(const char *nptr, char **endptr, int base, locale_t loc) {
    return strtoull(nptr, endptr, base);
}

long double strtold_l (const char *nptr, char **endptr, locale_t loc) {
    return strtold(nptr, endptr);
}

///////////////////////////////////////////////////////////////////////
// string.h declarations

int strcoll_l(const char* s1, const char* s2, locale_t loc) {
    return strcoll(s1, s2);
}

int strxfrm_l(char* dst, const char* src, size_t n, locale_t loc) {
    return strxfrm(dst, src, n);
}

///////////////////////////////////////////////////////////////////////
// time.h declarations

size_t strftime_l(char *s, size_t maxsize, const char *format,
                  const struct tm * timeptr, locale_t loc) {
    return strftime(s, maxsize, format, timeptr);
}

///////////////////////////////////////////////////////////////////////
// wchar.h declarations

int wcscoll_l(const wchar_t* s1, const wchar_t* s2, locale_t loc) {
    return wcscoll(s1, s2);
}

int wcsxfrm_l(wchar_t* dst, const wchar_t* src, size_t n, locale_t loc) {
    return wcsxfrm(dst, src, n);
}

///////////////////////////////////////////////////////////////////////
// wctype.h declarations

#define define_wchar_wrapper(name) \
  int name ## _l (wint_t c, locale_t loc) { \
    return name (c); \
  }

define_wchar_wrapper(iswspace)
define_wchar_wrapper(iswprint)
define_wchar_wrapper(iswcntrl)
define_wchar_wrapper(iswupper)
define_wchar_wrapper(iswlower)
define_wchar_wrapper(iswalpha)
define_wchar_wrapper(iswdigit)
define_wchar_wrapper(iswpunct)
define_wchar_wrapper(iswxdigit)
define_wchar_wrapper(iswblank)

wint_t towlower_l(wint_t c, locale_t loc) {
    return towlower(c);
}

wint_t towupper_l(wint_t c, locale_t loc) {
    return towupper(c);
}

///////////////////////////////////////////////////////////////////////
// locale.h declarations

#define LC_NULL_LOCALE  ((locale_t)0)

locale_t newlocale(int category_mask, const char* locale, locale_t base) {
    if (base != LC_NULL_LOCALE)
        return base;

    locale_t loc = calloc(1, sizeof(*loc));
    return loc;
}

locale_t duplocale(locale_t loc) {
    if (loc == LC_GLOBAL_LOCALE)
        return loc;
    if (loc == LC_NULL_LOCALE) {
        errno = EINVAL;
        return LC_NULL_LOCALE;
    }
    locale_t copy = calloc(1, sizeof(*loc));
    copy[0] = loc[0];
    return copy;
}

// Static mutable variable because setlocale() is supposed to return
// a pointer to a writable C string.
static char g_C_LOCALE_SETTING[] = "C";

char *setlocale(int category, const char *locale) {
    // Sanity check.
    if (!locale) {
        errno = EINVAL;
        return NULL;
    }
    // Only accept "", "C" or "POSIX", all equivalent on Android.
    if (strcmp(locale, "") && strcmp(locale, "C") && strcmp(locale, "POSIX")) {
        errno = EINVAL;
        return NULL;
    }
    return g_C_LOCALE_SETTING;
}

locale_t uselocale(locale_t loc) {
    // If 'loc' is LC_GLOBAL_LOCALE, should return the global locale set
    // through setlocale(). Since the implementation above doesn't modify
    // anything, just return LC_GLOBAL_LOCALE too.

    // If 'loc' is (locale_t)0, should return either LC_GLOBAL_LOCALE or
    // or the global locale if setlocale() has been called at least once.

    // Should return the previous value from a previous call, of
    // LC_GLOBAL_LOCALE.

    // So, in all cases, return LC_GLOBAL_LOCALE
    return LC_GLOBAL_LOCALE;
}

void freelocale(locale_t loc) {
    if (loc != LC_NULL_LOCALE && loc != LC_GLOBAL_LOCALE)
        free(loc);
}

static struct lconv g_C_LCONV[1] =  { {
    .decimal_point = ".",
    .thousands_sep = "",
    .grouping = "",
    .int_curr_symbol = "",
    .currency_symbol = "",
    .mon_decimal_point = "",
    .mon_thousands_sep = "",
    .mon_grouping = "",
    .positive_sign = "",
    .negative_sign = "",
    .int_frac_digits = CHAR_MAX,
    .frac_digits = CHAR_MAX,
    .p_cs_precedes = CHAR_MAX,
    .p_sep_by_space = CHAR_MAX,
    .n_cs_precedes = CHAR_MAX,
    .n_sep_by_space = CHAR_MAX,
    .p_sign_posn = CHAR_MAX,
    .n_sign_posn = CHAR_MAX,
    .int_p_cs_precedes = CHAR_MAX,
    .int_p_sep_by_space = CHAR_MAX,
    .int_n_cs_precedes = CHAR_MAX,
    .int_n_sep_by_space = CHAR_MAX,
    .int_p_sign_posn = CHAR_MAX,
    .int_n_sign_posn = CHAR_MAX,
} };

struct lconv* localeconv(void) {
    return g_C_LCONV;
}
