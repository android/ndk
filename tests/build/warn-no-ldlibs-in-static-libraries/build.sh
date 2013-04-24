#!/bin/sh

cd `dirname $0`
BUILD_OUTPUT=$($NDK/ndk-build "$@" 2>&1)
if [ $? != 0 ]; then
    echo "ERROR: Can't build test library!"
    printf "%s\n" "$BUILD_OUTPUT"
    exit 1
fi
printf "%s\n" "$BUILD_OUTPUT" | grep -q -e "WARNING:.*LOCAL_LDLIBS is always ignored for static libraries"
if [ $? != 0 ]; then
  echo "ERROR: Can't find LOCAL_LDLIBS warning in static library build output:"
  printf "%s\n" "$BUILD_OUTPUT"
  exit 1
fi
