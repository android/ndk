# serial 13

# Copyright (C) 2001, 2003, 2005, 2006, 2009 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Volker Borchert.
dnl Determine whether rename works for source file names with a trailing slash.
dnl The rename from SunOS 4.1.1_U1 doesn't.
dnl
dnl If it doesn't, then define RENAME_TRAILING_SLASH_BUG and arrange
dnl to compile the wrapper function.
dnl

AC_DEFUN([gl_FUNC_RENAME],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
 AC_CACHE_CHECK([whether rename is broken with a trailing slash],
  gl_cv_func_rename_trailing_slash_bug,
  [
    rm -rf conftest.d1 conftest.d2
    mkdir conftest.d1 ||
      AC_MSG_ERROR([cannot create temporary directory])
    AC_TRY_RUN([
#       include <stdio.h>
#       include <stdlib.h>
        int
        main ()
        {
          exit (rename ("conftest.d1/", "conftest.d2") ? 1 : 0);
        }
      ],
      gl_cv_func_rename_trailing_slash_bug=no,
      gl_cv_func_rename_trailing_slash_bug=yes,
      dnl When crosscompiling, assume rename is broken.
      gl_cv_func_rename_trailing_slash_bug=yes)

      rm -rf conftest.d1 conftest.d2
  ])
 AC_CACHE_CHECK([whether rename is broken when the destination exists],
  gl_cv_func_rename_dest_exists_bug,
  [
    case "$host_os" in
      mingw*) gl_cv_func_rename_dest_exists_bug=yes ;;
      *) gl_cv_func_rename_dest_exists_bug=no ;;
    esac
  ])
  if test $gl_cv_func_rename_trailing_slash_bug = yes ||
     test $gl_cv_func_rename_dest_exists_bug = yes; then
    AC_LIBOBJ([rename])
    AC_DEFINE([rename], [rpl_rename],
      [Define to rpl_rename if the replacement function should be used.])
    if test $gl_cv_func_rename_trailing_slash_bug = yes; then
      AC_DEFINE([RENAME_TRAILING_SLASH_BUG], [1],
	[Define if rename does not work for source file names with a trailing
	 slash, like the one from SunOS 4.1.1_U1.])
    fi
    if test $gl_cv_func_rename_dest_exists_bug = yes; then
      AC_DEFINE([RENAME_DEST_EXISTS_BUG], [1],
	[Define if rename does not work when the destination file exists,
	 as on Windows.])
    fi
    gl_PREREQ_RENAME
  fi
])

# Prerequisites of lib/rename.c.
AC_DEFUN([gl_PREREQ_RENAME], [:])
