#! /bin/sh

# edit this to taste; note that you can also override via the environment:
case "$CC" in
  "") CC=cc
esac

if test -f config.h; then :; else
  echo "Creating basic config.h..."
  cat >config.h <<'END_OF_CONFIG_H'
/* A bootstrap version of config.h, for systems which can't
   auto-configure due to a lack of a working sed.  If you are on
   a sufficiently odd machine you may need to hand-tweak this file.

   Regardless, once you get a working version of sed you really should
   re-build starting with a run of "configure", as the bootstrap
   version is almost certainly more crippled than it needs to be on
   your machine.
*/

#define PACKAGE "sed"
#define PACKAGE_BUGREPORT "bug-gnu-utils@gnu.org"
#define VERSION "4.2.1-boot"
#define SED_FEATURE_VERSION "4.2"
#define BOOTSTRAP 1

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

/* Define if your compiler/headers don't support const. */
#undef const
#define __getopt_argv_const const

/* Define if headers have no definition.  */
/* #define mbstate_t int */
#define HAVE_WCHAR_H 1
#define HAVE_MBRTOWC 1

/* Toggle if you encounter errors in lib/mkstemp.c.  */
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#undef HAVE_SYS_FILE_H
#undef HAVE_IO_H

/* Emulate stdbool.h.  */
#define bool int
#define true 1
#define false 0

/* Avoid uselessly complicated gnulib stuff.  */
#define _LOCALCHARSET_H
#define locale_charset()	"C"

/* All other config.h.in options intentionally omitted.  Report as a
   bug if you need extra "#define"s in here. */

END_OF_CONFIG_H

  cat > conftest.c << \EOF
#define size_t unsigned
#include <sys/types.h>
#include <stdio.h>

size_t s;
EOF
  if $CC -c conftest.c -o conftest.o > /dev/null 2>&1 ; then
    echo '#define size_t unsigned' >> config.h
    echo checking for size_t... no
  else
    echo checking for size_t... yes
  fi

  cat > conftest.c << \EOF
#define ssize_t int
#include <sys/types.h>
#include <stdio.h>

ssize_t s;
EOF
  if $CC -c conftest.c -o conftest.o > /dev/null 2>&1 ; then
    echo '#define ssize_t int' >> config.h
    echo checking for ssize_t... no
  else
    echo checking for ssize_t... yes
  fi

  cat > conftest.c << \EOF
void *foo;

EOF
  if $CC -c conftest.c -o conftest.o > /dev/null 2>&1 ; then
    echo checking for void *... yes
  else
    echo '#define VOID char' >> config.h
    echo checking for void *... no
  fi

  rm -f conftest.*

  cat >> config.h << \EOF
#include <sys/types.h>
#include <stdio.h>
EOF

fi

# tell the user what we're doing from here on...
set -x -e

# the ``|| exit 1''s are for fail-stop; set -e doesn't work on some systems

rm -f lib/*.o sed/*.o sed/sed
cd lib || exit 1
cp alloca.in.h alloca.h || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c alloca.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c copy-acl.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c error.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c exitfail.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c file-has-acl.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c getdelim.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c getline.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c getopt.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c getopt1.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c malloc.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c mkstemp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c obstack.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c quote.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c quotearg.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c regex.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c set-mode-acl.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c strerror.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c strverscmp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c tempname.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c xalloc-die.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -c xmalloc.c || exit 1

cd ../sed || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c sed.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c fmt.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c compile.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c execute.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c mbcs.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c regexp.c || exit 1
${CC} -DHAVE_CONFIG_H -I.. -I. -I../lib -c utils.c || exit 1

${CC} -o sed *.o ../lib/*.o || exit 1
