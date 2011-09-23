# sys_stat_h.m4 serial 10   -*- Autoconf -*-
dnl Copyright (C) 2006-2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Eric Blake.
dnl Test whether <sys/stat.h> contains lstat and mkdir or must be substituted.

AC_DEFUN([gl_HEADER_SYS_STAT_H],
[
  AC_REQUIRE([gl_SYS_STAT_H_DEFAULTS])

  dnl Check for lstat.  Systems that lack it (mingw) also lack symlinks, so
  dnl stat is a good replacement.
  AC_CHECK_FUNCS_ONCE([lstat])
  if test $ac_cv_func_lstat = yes; then
    HAVE_LSTAT=1
  else
    HAVE_LSTAT=0
  fi
  AC_SUBST([HAVE_LSTAT])

  dnl For the mkdir substitute.
  AC_REQUIRE([AC_C_INLINE])

  dnl Check for broken stat macros.
  AC_REQUIRE([AC_HEADER_STAT])

  gl_CHECK_NEXT_HEADERS([sys/stat.h])
  SYS_STAT_H='sys/stat.h'
  AC_SUBST([SYS_STAT_H])

  dnl Define types that are supposed to be defined in <sys/types.h> or
  dnl <sys/stat.h>.
  AC_CHECK_TYPE([nlink_t], [],
    [AC_DEFINE([nlink_t], [int],
       [Define to the type of st_nlink in struct stat, or a supertype.])],
    [#include <sys/types.h>
     #include <sys/stat.h>])

]) # gl_HEADER_SYS_STAT_H

AC_DEFUN([gl_SYS_STAT_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_SYS_STAT_H_DEFAULTS])
  GNULIB_[]m4_translit([$1],[abcdefghijklmnopqrstuvwxyz./-],[ABCDEFGHIJKLMNOPQRSTUVWXYZ___])=1
])

AC_DEFUN([gl_SYS_STAT_H_DEFAULTS],
[
  GNULIB_LCHMOD=0; AC_SUBST([GNULIB_LCHMOD])
  GNULIB_LSTAT=0;  AC_SUBST([GNULIB_LSTAT])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_LCHMOD=1;   AC_SUBST([HAVE_LCHMOD])
  REPLACE_LSTAT=0; AC_SUBST([REPLACE_LSTAT])
  REPLACE_MKDIR=0; AC_SUBST([REPLACE_MKDIR])
])
