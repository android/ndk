#!/bin/sh

set -e
export LANG=C
export LC_ALL=C

PROGNAME=android_support_unittests
EXECUTABLE=${EXECUTABLE:-/tmp/ndk-$USER/$PROGNAME}

CXX=${CXX:-g++}
CXXFLAGS="-Werror -Wall"
LDFLAGS=
LINK_LIBS="-lpthread"

SOURCES="\
  stdio_unittest.cc \
  wchar_unittest.cc \
"

MINITEST_DIR=$(dirname "$0")
MINITEST_INCLUDES="-I$MINITEST_DIR"
MINITEST_SOURCES="$MINITEST_DIR/minitest/minitest.cc"

CXXFLAGS="$CXXFLAGS $MINITEST_INCLUDES"
SOURCES="$SOURCES $MINITEST_SOURCES"

mkdir -p $(dirname "$EXECUTABLE")
$CXX -o "$EXECUTABLE" $SOURCES $CXXFLAGS $LINK_LIBS $LDFLAGS
"$EXECUTABLE"

