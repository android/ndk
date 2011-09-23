# btowc.m4 serial 4
dnl Copyright (C) 2008-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_BTOWC],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])

  AC_CHECK_FUNCS_ONCE([btowc])
  if test $ac_cv_func_btowc = no; then
    HAVE_BTOWC=0
  else

    dnl IRIX 6.5 btowc(EOF) is 0xFF, not WEOF.
    AC_REQUIRE([AC_PROG_CC])
    AC_REQUIRE([gt_LOCALE_FR])
    AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
    AC_CACHE_CHECK([whether btowc(EOF) is correct],
      [gl_cv_func_btowc_eof],
      [
        dnl Initial guess, used when cross-compiling or when no suitable locale
        dnl is present.
changequote(,)dnl
        case "$host_os" in
                 # Guess no on IRIX.
          irix*) gl_cv_func_btowc_eof="guessing no" ;;
                 # Guess yes otherwise.
          *)     gl_cv_func_btowc_eof="guessing yes" ;;
        esac
changequote([,])dnl
        if test $LOCALE_FR != none; then
          AC_TRY_RUN([
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
int main ()
{
  if (setlocale (LC_ALL, "$LOCALE_FR") != NULL)
    {
      if (btowc (EOF) != WEOF)
        return 1;
    }
  return 0;
}],
            [gl_cv_func_btowc_eof=yes],
            [gl_cv_func_btowc_eof=no],
            [:])
        fi
      ])
    case "$gl_cv_func_btowc_eof" in
      *yes) ;;
      *) REPLACE_BTOWC=1 ;;
    esac
  fi
  if test $HAVE_BTOWC = 0 || test $REPLACE_BTOWC = 1; then
    gl_REPLACE_WCHAR_H
    AC_LIBOBJ([btowc])
    gl_PREREQ_BTOWC
  fi
])

# Prerequisites of lib/btowc.c.
AC_DEFUN([gl_PREREQ_BTOWC], [
  :
])
