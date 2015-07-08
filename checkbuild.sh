#!/bin/bash
set -e

cd "$( dirname "${BASH_SOURCE[0]}" )"
if [ -z "$ANDROID_BUILD_TOP" ]; then
  export ANDROID_BUILD_TOP=$(./realpath ..)
fi

PACKAGE_DIR_ARG=
if [ -n "$DIST_DIR" ]; then
    PACKAGE_DIR_ARG="--package-dir=$(./realpath $DIST_DIR)"
elif [[ "$*" != *--package-dir=* ]]; then
    PACKAGE_DIR_ARG="--package-dir=$(./realpath ../out/ndk)"
fi

set +e
echo "$*" | grep -w -- '--systems=windows' >/dev/null
RESULT=$?
set -e

# Skip building the target modules if we're building for just Windows or Darwin.
if [ $RESULT -eq 0 -o "$(uname)" == "Darwin" ]; then
    JUST_HOST=true
fi

build/tools/dev-cleanup.sh

COMMON_ARGS="$(./realpath ../toolchain) --verbose --try-64 $PACKAGE_DIR_ARG"

if [ "$JUST_HOST" == "true" ]; then
    build/tools/build-host-prebuilts.sh \
        $COMMON_ARGS \
        --ndk-dir=$(./realpath .) \
        $*
else
    build/tools/rebuild-all-prebuilt.sh \
      $COMMON_ARGS \
      $*
fi
