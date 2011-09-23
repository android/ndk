dnl Process this file with -*- autoconf -*- to produce a configure script.
AC_INIT(sed, 4.2.1, bug-gnu-utils@gnu.org, sed)
AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_SRCDIR([sed/sed.c])
AM_CONFIG_HEADER(config.h:config_h.in)
AC_PREREQ(2.60)
AM_INIT_AUTOMAKE

SED_FEATURE_VERSION=4.2
AC_DEFINE_UNQUOTED(SED_FEATURE_VERSION, "$SED_FEATURE_VERSION",
  [Define to the version of GNU sed whose features are supported by this sed.])
AC_SUBST(SED_FEATURE_VERSION)

COPYRIGHT_YEAR=2009
AC_DEFINE_UNQUOTED(COPYRIGHT_YEAR, $COPYRIGHT_YEAR,
  [Define to the copyright year printed by --version.])
AC_SUBST(COPYRIGHT_YEAR)

AC_PROG_CC
gl_EARLY
gl_INIT
gl_DISABLE_THREADS
AC_SYS_LONG_FILE_NAMES
AC_CACHE_CHECK([whether -lcP is needed], [sed_cv_libcp_needed], [
AC_TRY_RUN([
#include <stdio.h>
#include <errno.h>

int main()
{
  FILE *fp;
  int result;
  errno = 0;
  fp = fopen ("conftest.c", "r");
  if (!fp) return 0;	   /* error, assume not needed */
  result = fflush (fp) == EOF && errno == 0;
  fclose (fp);
  return result;
}], [sed_cv_libcp_needed=no],
    [sed_cv_libcp_needed=yes],
    [sed_cv_libcp_needed="assuming no"])
])
if test "$sed_cv_libcp_needed" = yes; then
  LIBS="-lcP $LIBS"
fi

AC_HEADER_DIRENT
AC_CHECK_HEADERS_ONCE(io.h limits.h locale.h stdarg.h alloca.h stddef.h
		      errno.h wchar.h wctype.h sys/file.h mcheck.h, [], [],
		 [AC_INCLUDES_DEFAULT])
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)

AC_FUNC_VPRINTF

AC_DEFUN([AM_MKINSTALLDIRS], [MKINSTALLDIRS="$mkdir_p" AC_SUBST(MKINSTALLDIRS)])
AM_GNU_GETTEXT_VERSION(0.15)
AM_GNU_GETTEXT([external])

AC_CHECK_FUNCS_ONCE(isatty bcopy bzero isascii memcpy memset strchr strtoul
		    popen pathconf fchown fchmod setlocale)

AC_ARG_ENABLE(i18n,
[  --disable-i18n          disable internationalization (default=enabled)], ,
enable_i18n=yes)
if test "x$enable_i18n" = xno; then
  ac_cv_func_wcscoll=false
fi

AC_ARG_ENABLE(regex-tests, 
[  --enable-regex-tests    enable regex matcher regression tests (default=yes)],
[if test "x$with_included_regex" = xno; then
  enable_regex_tests=no
fi],
enable_regex_tests=$with_included_regex)

AM_CONDITIONAL(TEST_REGEX, test "x$enable_regex_tests" = xyes)
if test "x$enable_regex_tests" = xyes; then
  AC_DEFINE_UNQUOTED(_REGEX_RE_COMP, 1,
    [Include BSD functions in regex, used by the testsuite])
fi

# Check whether we are able to follow symlinks
AC_CHECK_FUNC(lstat, have_lstat=yes)
AC_CHECK_FUNC(readlink, have_readlink=yes)
if test "x$have_lstat" = xyes -a "x$have_readlink" = xyes; then
   AC_DEFINE(ENABLE_FOLLOW_SYMLINKS, ,[Follow symlinks when processing in place])
fi

# Determine whether we should run UTF-8 tests by checking if cyrillic
# letters are case-folded properly.  The test for UTF-8 locales (bot
# in general and specifically for a Russian one) is a bit weak, but it
# should match exactly what is done in the testsuite.  In fact, sed's
# logic is portable (though testing it requires care) so it is enough to
# have a couple of platforms where these tests pass.  Right now, only
# Windows and HP/UX do not support the tests.
AC_MSG_CHECKING([whether UTF-8 case folding tests should pass])
AC_TRY_RUN([
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#endif

int test(void)
{
  char in[] = "\xD0\xB4";
  char good[] = "\xD0\x94";
  char out[10];
  wchar_t in_wc, good_wc;
  if (mbtowc (&in_wc, in, 3) == -1)
    return 0;
  if (mbtowc (&good_wc, good, 3) == -1)
    return 0;
  if (towupper (in_wc) != good_wc)
    return 0;
  if (wctomb (out, good_wc) != 2)
    return 0;
  if (memcmp (out, good, 2))
    return 0;
  return 1;
}

int main()
{
  char *old;
  int len;

  /* Try hardcoding a Russian UTF-8 locale.  If the name "ru_RU.UTF-8"
     is invalid, use setlocale again just to get the current locale.  */
  old = setlocale (LC_CTYPE, "ru_RU.UTF-8");
  if (old)
    {
      if (test())
        exit (0);
    }
  else
    old = setlocale (LC_CTYPE, "C");

  /* Maybe cyrillic case folding is implemented for all UTF-8 locales.
     If the current locale is not UTF-8, the test will be skipped.  */
  len = strlen (old);
  if ((len > 6 && !strcmp (old + len - 6, ".UTF-8"))
      || (len > 6 && !strcmp (old + len - 6, ".utf-8"))
      || (len > 5 && !strcmp (old + len - 5, ".UTF8"))
      || (len > 5 && !strcmp (old + len - 5, ".utf8")))

    /* ok */
    ;
  else
    exit (1);

  /* Run the test in the detected UTF-8 locale.  */
  setlocale (LC_CTYPE, old);
  exit (!test ());
}
], [AC_MSG_RESULT([yes]); XFAIL_TESTS=],
   [AC_MSG_RESULT([no]); XFAIL_TESTS='utf8-1 utf8-2 utf8-3 utf8-4'],
   [AC_MSG_RESULT([don't care (cross compiling)]); XFAIL_TESTS=])

# Under MinGW, the bsd.sh test fails because of the EOF character (^Z).
case $host in
  *mingw*) XFAIL_TESTS="$XFAIL_TESTS bsd" ;;
  *) ;;
esac
AC_SUBST([XFAIL_TESTS])

AC_CONFIG_FILES([bootstrap.sh], chmod +x bootstrap.sh)
AC_CONFIG_FILES([testsuite/version.good:testsuite/version.gin])
AC_CONFIG_FILES([Makefile doc/Makefile \
lib/Makefile sed/Makefile testsuite/Makefile \
po/Makefile.in])
AC_OUTPUT
