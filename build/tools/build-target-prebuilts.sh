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

ARCHS="$DEFAULT_ARCHS"
register_var_option "--arch=<list>" ARCHS "List of target archs to build for"

NO_GEN_PLATFORMS=
register_var_option "--no-gen-platforms" NO_GEN_PLATFORMS "Don't generate platforms/ directory, use existing one"

GCC_VERSION_LIST="default" # it's arch defined by default so use default keyword
register_var_option "--gcc-version-list=<vers>" GCC_VERSION_LIST "GCC version list (libgnustl should be built per each GCC version)"

LLVM_VERSION=
register_var_option "--llvm-version=<vers>" LLVM_VERSION "LLVM version"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package toolchain into this directory"

VISIBLE_LIBGNUSTL_STATIC=
register_var_option "--visible-libgnustl-static" VISIBLE_LIBGNUSTL_STATIC "Do not use hidden visibility for libgnustl_static.a"

register_jobs_option

register_try64_option

PROGRAM_PARAMETERS="<toolchain-src-dir>"
PROGRAM_DESCRIPTION="This script can be used to rebuild all the target NDK prebuilts at once."

extract_parameters "$@"

# Pickup one GCC_VERSION for the cases where we want only one build
# That's actually all cases except libgnustl where we are building for each GCC version
GCC_VERSION=
if [ "$GCC_VERSION_LIST" != "default" ]; then
   GCC_VERSIONS=$(commas_to_spaces $GCC_VERSION_LIST)
   GCC_VERSION=${GCC_VERSIONS%% *}
fi

# Use DEFAULT_LLVM_VERSION to build targets unless we want to build with some particular version
if [ -z "$GCC_VERSION_LIST" -a -z "$LLVM_VERSION" ]; then
   LLVM_VERSION=$DEFAULT_LLVM_VERSION
fi

if [ ! -z "$LLVM_VERSION" ]; then
   BUILD_TOOLCHAIN="--llvm-version=$LLVM_VERSION"
elif [ ! -z "$GCC_VERSION" ]; then
   BUILD_TOOLCHAIN="--gcc-version=$GCC_VERSION"
fi

# Check toolchain source path
SRC_DIR="$PARAMETERS"
check_toolchain_src_dir "$SRC_DIR"
SRC_DIR=`cd $SRC_DIR; pwd`

# Now we can do the build
BUILDTOOLS=$ANDROID_NDK_ROOT/build/tools

PACKAGE_FLAGS=
if [ "$PACKAGE_DIR" ]; then
    PACKAGE_FLAGS="--package-dir=$PACKAGE_DIR"
fi

ARCHS=$(commas_to_spaces $ARCHS)

FLAGS=
if [ "$DRYRUN" = "yes" ]; then
    FLAGS=$FLAGS" --dryrun"
fi
if [ "$VERBOSE" = "yes" ]; then
    FLAGS=$FLAGS" --verbose"
fi
if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
    FLAGS=$FLAGS" --package-dir=\"$PACKAGE_DIR\""
fi
FLAGS=$FLAGS" -j$NUM_JOBS"

if [ "$TRY64" = "yes" ]; then
    FLAGS=$FLAGS" --try-64"
fi

FLAGS=$FLAGS" --ndk-dir=\"$NDK_DIR\""
ABIS=$(convert_archs_to_abis $ARCHS)

dump "Building $ABIS gabi++ binaries..."
run $BUILDTOOLS/build-cxx-stl.sh --stl=gabi++ --abis="$ABIS" $FLAGS --with-debug-info $BUILD_TOOLCHAIN
fail_panic "Could not build gabi++ with debug info!"

dump "Building $ABIS stlport binaries..."
run $BUILDTOOLS/build-cxx-stl.sh --stl=stlport --abis="$ABIS" $FLAGS --with-debug-info $BUILD_TOOLCHAIN
fail_panic "Could not build stlport with debug info!"

dump "Building $ABIS libc++ binaries... with libc++abi"
run $BUILDTOOLS/build-cxx-stl.sh --stl=libc++-libc++abi --abis="$ABIS" $FLAGS --with-debug-info $BUILD_TOOLCHAIN
fail_panic "Could not build libc++ with libc++abi and debug info!"

# workaround issues in libc++/libc++abi for x86 and mips
for abi in $ABIS; do
  case $abi in
     x86|x86_64|mips|mips32r6|mips64)
  dump "Rebuilding $abi libc++ binaries... with gabi++"
  run $BUILDTOOLS/build-cxx-stl.sh --stl=libc++-gabi++ --abis=$abi $FLAGS --with-debug-info $BUILD_TOOLCHAIN
  esac
done

if [ "$PACKAGE_DIR" ]; then
    dump "Done, see $PACKAGE_DIR"
else
    dump "Done"
fi

exit 0
