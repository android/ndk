# wcrtomb.m4 serial 4
dnl Copyright (C) 2008-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_WCRTOMB],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])

  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  gl_MBSTATE_T_BROKEN
  if test $REPLACE_MBSTATE_T = 1; then
    REPLACE_WCRTOMB=1
  fi
  AC_CHECK_FUNCS_ONCE([wcrtomb])
  if test $ac_cv_func_wcrtomb = no; then
    HAVE_WCRTOMB=0
  fi
  if test $HAVE_WCRTOMB != 0 && test $REPLACE_WCRTOMB != 1; then
    dnl On AIX 4.3, OSF/1 5.1 and Solaris 10, wcrtomb (NULL, 0, NULL) sometimes
    dnl returns 0 instead of 1.
    AC_REQUIRE([AC_PROG_CC])
    AC_REQUIRE([gt_LOCALE_FR])
    AC_REQUIRE([gt_LOCALE_FR_UTF8])
    AC_REQUIRE([gt_LOCALE_JA])
    AC_REQUIRE([gt_LOCALE_ZH_CN])
    AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
    AC_CACHE_CHECK([whether wcrtomb return value is correct],
      [gl_cv_func_wcrtomb_retval],
      [
        dnl Initial guess, used when cross-compiling or when no suitable locale
        dnl is present.
changequote(,)dnl
        case "$host_os" in
                                   # Guess no on AIX 4, OSF/1 and Solaris.
          aix4* | osf* | solaris*) gl_cv_func_wcrtomb_retval="guessing no" ;;
                                   # Guess yes otherwise.
          *)                       gl_cv_func_wcrtomb_retval="guessing yes" ;;
        esac
changequote([,])dnl
        if test $LOCALE_FR != none || test $LOCALE_FR_UTF8 != none || test $LOCALE_JA != none || test $LOCALE_ZH_CN != none; then
          AC_TRY_RUN([
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
int main ()
{
  if (setlocale (LC_ALL, "$LOCALE_FR") != NULL)
    {
      if (wcrtomb (NULL, 0, NULL) != 1)
        return 1;
    }
  if (setlocale (LC_ALL, "$LOCALE_FR_UTF8") != NULL)
    {
      if (wcrtomb (NULL, 0, NULL) != 1)
        return 1;
    }
  if (setlocale (LC_ALL, "$LOCALE_JA") != NULL)
    {
      if (wcrtomb (NULL, 0, NULL) != 1)
        return 1;
    }
  if (setlocale (LC_ALL, "$LOCALE_ZH_CN") != NULL)
    {
      if (wcrtomb (NULL, 0, NULL) != 1)
        return 1;
    }
  return 0;
}],
            [gl_cv_func_wcrtomb_retval=yes],
            [gl_cv_func_wcrtomb_retval=no],
            [:])
        fi
      ])
    case "$gl_cv_func_wcrtomb_retval" in
      *yes) ;;
      *) REPLACE_WCRTOMB=1 ;;
    esac
  fi
  if test $HAVE_WCRTOMB = 0 || test $REPLACE_WCRTOMB = 1; then
    gl_REPLACE_WCHAR_H
    AC_LIBOBJ([wcrtomb])
    gl_PREREQ_WCRTOMB
  fi
])

# Prerequisites of lib/wcrtomb.c.
AC_DEFUN([gl_PREREQ_WCRTOMB], [
  :
])
