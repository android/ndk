// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBCXX_SUPPORT_ANDROID_LOCALE_H
#define LLVM_LIBCXX_SUPPORT_ANDROID_LOCALE_H

#define lconv  __libc_lconv
#define localeconv  __libc_localeconv
#include_next <locale.h>
#undef lconv
#undef localeconv
#include <xlocale.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LC_CTYPE_MASK  (1 << LC_CTYPE)
#define LC_NUMERIC_MASK (1 << LC_NUMERIC)
#define LC_TIME_MASK (1 << LC_TIME)
#define LC_COLLATE_MASK (1 << LC_COLLATE)
#define LC_MONETARY_MASK (1 << LC_MONETARY)
#define LC_MESSAGES_MASK (1 << LC_MESSAGES)
#define LC_PAPER_MASK (1 << LC_PAPER)
#define LC_NAME_MASK (1 << LC_NAME)
#define LC_ADDRESS_MASK (1 << LC_ADDRESS)
#define LC_TELEPHONE_MASK (1 << LC_TELEPHONE)
#define LC_MEASUREMENT_MASK (1 << LC_MEASUREMENT)
#define LC_IDENTIFICATION_MASK (1 << LC_IDENTIFICATION)

#define LC_ALL_MASK (LC_CTYPE_MASK \
                     | LC_NUMERIC_MASK \
                     | LC_TIME_MASK \
                     | LC_COLLATE_MASK \
                     | LC_MONETARY_MASK \
                     | LC_MESSAGES_MASK \
                     | LC_PAPER_MASK \
                     | LC_NAME_MASK \
                     | LC_ADDRESS_MASK \
                     | LC_TELEPHONE_MASK \
                     | LC_MEASUREMENT_MASK \
                     | LC_IDENTIFICATION_MASK \
                     )

extern locale_t newlocale(int, const char*, locale_t);
extern locale_t uselocale(locale_t);
extern void     freelocale(locale_t);

#define LC_GLOBAL_LOCALE    ((locale_t) -1L)

struct lconv {
    char* decimal_point;       /* Decimal point character */
    char* thousands_sep;       /* Thousands separator */
    char* grouping;            /* Grouping */
    char* int_curr_symbol;
    char* currency_symbol;
    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;
    char  int_frac_digits;
    char  frac_digits;
    char  p_cs_precedes;
    char  p_sep_by_space;
    char  n_cs_precedes;
    char  n_sep_by_space;
    char  p_sign_posn;
    char  n_sign_posn;
    /* ISO-C99 */
    char  int_p_cs_precedes;
    char  int_p_sep_by_space;
    char  int_n_cs_precedes;
    char  int_n_sep_by_space;
    char  int_p_sign_posn;
    char  int_n_sign_posn;
};

struct lconv* localeconv(void);

// Used to implement the std::ctype<char> specialization.
extern const char * const __ctype_c_mask_table;
// TODO(ajwong): Make this based on some exported bionic constant.
const int __ctype_c_mask_table_size = 256;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LLVM_LIBCXX_SUPPORT_ANDROID_LOCALE_H
