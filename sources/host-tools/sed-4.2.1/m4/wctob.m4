# wctob.m4 serial 4
dnl Copyright (C) 2008-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_WCTOB],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])

  AC_CHECK_FUNCS_ONCE([wctob])
  if test $ac_cv_func_wctob = no; then
    HAVE_DECL_WCTOB=0
    gl_REPLACE_WCHAR_H
    AC_LIBOBJ([wctob])
    gl_PREREQ_WCTOB
  else

    dnl Solaris 9 has the wctob() function but it does not work.
    AC_REQUIRE([AC_PROG_CC])
    AC_REQUIRE([gt_LOCALE_FR])
    AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
    AC_CACHE_CHECK([whether wctob works],
      [gl_cv_func_wctob_works],
      [
        dnl Initial guess, used when cross-compiling or when no suitable locale
        dnl is present.
changequote(,)dnl
        case "$host_os" in
            # Guess no on Solaris <= 9.
          solaris2.[1-9] | solaris2.[1-9].*)
            gl_cv_func_wctob_works="guessing no" ;;
            # Guess yes otherwise.
          *) gl_cv_func_wctob_works="guessing yes" ;;
        esac
changequote([,])dnl
        if test $LOCALE_FR != none; then
          AC_TRY_RUN([
#include <locale.h>
#include <string.h>
#include <wchar.h>
int main ()
{
  if (setlocale (LC_ALL, "$LOCALE_FR") != NULL)
    {
      wchar_t wc;

      if (mbtowc (&wc, "\374", 1) == 1)
        if (wctob (wc) != (unsigned char) '\374')
          return 1;
    }
  return 0;
}],
            [gl_cv_func_wctob_works=yes],
            [gl_cv_func_wctob_works=no],
            [:])
        fi
      ])
    case "$gl_cv_func_wctob_works" in
      *yes) ;;
      *) REPLACE_WCTOB=1 ;;
    esac
    if test $REPLACE_WCTOB = 1; then
      gl_REPLACE_WCHAR_H
      AC_LIBOBJ([wctob])
      gl_PREREQ_WCTOB
    else

      dnl IRIX 6.5 has the wctob() function but does not declare it.
      AC_CHECK_DECLS([wctob], [], [], [
/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be included
   before <wchar.h>.  */
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
])
      if test $ac_cv_have_decl_wctob != yes; then
        HAVE_DECL_WCTOB=0
        gl_REPLACE_WCHAR_H
      fi
    fi
  fi
])

# Prerequisites of lib/wctob.c.
AC_DEFUN([gl_PREREQ_WCTOB], [
  :
])
