#!/bin/sh
#
# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Rebuild all target-specific prebuilts
#

PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "NDK installation directory"

ARCHS=$DEFAULT_ARCHS
register_var_option "--arch=<list>" ARCHS "List of target archs to build for"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package toolchain into this directory"

register_jobs_option

PROGRAM_PARAMETERS="<toolchain-src-dir>"
PROGRAM_DESCRIPTION=\
"This script can be used to rebuild all the target NDK prebuilts at once.
You need to give it the path to the toolchain source directory, as
downloaded by the 'download-toolchain-sources.sh' dev-script."

extract_parameters "$@"

# Check toolchain source path
SRC_DIR="$PARAMETERS"
check_toolchain_src_dir "$SRC_DIR"

# Now we can do the build
BUILDTOOLS=$ANDROID_NDK_ROOT/build/tools

dump "Building platforms and samples..."
PACKAGE_FLAGS=
if [ "$PACKAGE_DIR" ]; then
    PACKAGE_FLAGS="--package-dir=$PACKAGE_DIR"
fi

run $BUILDTOOLS/gen-platforms.sh --samples --fast-copy --dst-dir=$NDK_DIR --ndk-dir=$NDK_DIR --arch=$(spaces_to_commas $ARCHS) $PACKAGE_FLAGS
fail_panic "Could not generate platforms and samples directores!"

ARCHS=$(commas_to_spaces $ARCHS)

FLAGS=
if [ "$VERBOSE" = "yes" ]; then
    FLAGS=$FLAGS" --verbose"
fi
if [ "$VERBOSE2" = "yes" ]; then
    FLAGS=$FLAGS" --verbose"
fi
if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
    FLAGS=$FLAGS" --package-dir=\"$PACKAGE_DIR\""
fi
FLAGS=$FLAGS" -j$NUM_JOBS"

# First, gdbserver
for ARCH in $ARCHS; do
    dump "Building $ARCH gdbserver binaries..."
    GDB_TOOLCHAIN=$(get_default_toolchain_name_for_arch $ARCH)
    run $BUILDTOOLS/build-gdbserver.sh "$SRC_DIR" "$NDK_DIR" "$GDB_TOOLCHAIN" $FLAGS
    fail_panic "Could not build $ARCH gdb-server!"
done

FLAGS=$FLAGS" --ndk-dir=\"$NDK_DIR\""

dump "Building gabi++ binaries..."
run $BUILDTOOLS/build-gabi++.sh $FLAGS
fail_panic "Could not build gabi++!"

dump "Building stlport binaries..."
run $BUILDTOOLS/build-stlport.sh $FLAGS
fail_panic "Could not build stlport!"

dump "Building gnustl binaries..."
run $BUILDTOOLS/build-gnu-libstdc++.sh $FLAGS "$SRC_DIR"
fail_panic "Could not build gnustl!"

if [ "$PACKAGE_DIR" ]; then
    dump "Done, see $PACKAGE_DIR"
else
    dump "Done"
fi

exit 0
