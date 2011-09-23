# serial 3   -*- Autoconf -*-
# Copyright (C) 2006, 2007, 2009 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# From Jim Meyering
# Provide <selinux/selinux.h>, if necessary.

AC_DEFUN([gl_HEADERS_SELINUX_SELINUX_H],
[
  AC_REQUIRE([gl_LIBSELINUX])
  AC_CHECK_HEADERS([selinux/selinux.h],
		   [SELINUX_SELINUX_H=],
		   [SELINUX_SELINUX_H=selinux/selinux.h])
  AC_SUBST([SELINUX_SELINUX_H])

  case "$ac_cv_search_setfilecon:$ac_cv_header_selinux_selinux_h" in
    no:*) # already warned
      ;;
    *:no)
      AC_MSG_WARN([libselinux was found but selinux/selinux.h is missing.])
      AC_MSG_WARN([AC_PACKAGE_NAME will be compiled without SELinux support.])
  esac
])

AC_DEFUN([gl_LIBSELINUX],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([AC_CANONICAL_BUILD])
  LIB_SELINUX=
  gl_save_LIBS=$LIBS
  AC_SEARCH_LIBS([setfilecon], [selinux],
                 [test "$ac_cv_search_setfilecon" = "none required" ||
                  LIB_SELINUX=$ac_cv_search_setfilecon])
  AC_SUBST([LIB_SELINUX])
  LIBS=$gl_save_LIBS

  # Warn if SELinux is found but libselinux is absent;
  if test "$ac_cv_search_setfilecon" = no &&
     test "$host" = "$build" && test -d /selinux; then
    AC_MSG_WARN([This system supports SELinux but libselinux is missing.])
    AC_MSG_WARN([AC_PACKAGE_NAME will be compiled without SELinux support.])
  fi
])
