#!/bin/sh

cd `dirname $0`
BUILD_OUTPUT=$($NDK/ndk-build "$@" 2>&1)
# Note: the build can fail because it is ill-specific, so don't panic
#       and just look for the expected warning.
printf "%s\n" "$BUILD_OUTPUT" | grep -q -e "WARNING:.*non-system libraries in linker flags: -lfoo"
if [ $? != 0 ]; then
  echo "ERROR: Can't find warning about non-system libraries in linker flags:"
  printf "%s\n" "$BUILD_OUTPUT"
  exit 1
fi
