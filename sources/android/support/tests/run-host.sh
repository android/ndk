#!/bin/sh

set -e
export LANG=C
export LC_ALL=C

PROGDIR=$(dirname "$0")

EXENAME=android_support_unittests
EXECUTABLE=${EXECUTABLE:-/tmp/ndk-$USER/$EXENAME}

CXX=${CXX:-g++}
CXXFLAGS="-Werror -Wall"
LDFLAGS=
LINK_LIBS=""

# NOTE: libdl_unittest.cc can not run on Ubuntu because dladdr is missing

SOURCES="\
  ctype_unittest.cc \
  math_unittest.cc \
  stdio_unittest.cc \
  wchar_unittest.cc \
"

MINITEST_INCLUDES="-I."
MINITEST_SOURCES="minitest/minitest.cc"

CXXFLAGS="$CXXFLAGS $MINITEST_INCLUDES"
SOURCES="$SOURCES $MINITEST_SOURCES"

mkdir -p $(dirname "$EXECUTABLE")
cd $PROGDIR
$CXX -o "$EXECUTABLE" $SOURCES $CXXFLAGS $LINK_LIBS $LDFLAGS
"$EXECUTABLE"

