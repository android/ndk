#!/bin/sh

# This script is used to check that using absolute paths in
# LOCAL_SRC_PATHS works properly.
CUR_DIR=$(cd $(dirname "$0") && pwd)
echo $NDK/ndk-build "$@" FOO_PATH="$CUR_DIR/src"
$NDK/ndk-build "$@" FOO_PATH="$CUR_DIR/src"
