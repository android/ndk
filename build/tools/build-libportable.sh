#!/bin/sh
#
# Copyright (C) 2013 The Android Open Source Project
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
#  This shell script is used to rebuild the libportable binaries from
#  their sources. It requires a working NDK installation.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
. `dirname $0`/builder-funcs.sh

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Rebuild libportable.a for the Android NDK.

This script builds libportable.a for app link with portable header.

This requires a platform.git/development and a temporary NDK installation
containing platforms and toolchain binaries for all target architectures.

By default, this will try with the current NDK directory, unless
you use the --ndk-dir=<path> option.

The output will be placed in appropriate sub-directories of
<ndk>/$LIBPORTABLE_SUBDIR, but you can override this with the --out-dir=<path>
option.
"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Put prebuilt tarballs into <path>."

NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "Specify NDK root path for the build."

BUILD_DIR=
OPTION_BUILD_DIR=
register_var_option "--build-dir=<path>" OPTION_BUILD_DIR "Specify temporary build dir."

OUT_DIR=
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory directly."

ABIS="$PREBUILT_ABIS"
register_var_option "--abis=<list>" ABIS "Specify list of target ABIs."

NO_MAKEFILE=
register_var_option "--no-makefile" NO_MAKEFILE "Do not use makefile to speed-up build"

VISIBLE_LIBPORTABLE_STATIC=
register_var_option "--visible-libportable-static" VISIBLE_LIBPORTABLE_STATIC "Do not use hidden visibility for libportable.a"

GCC_VERSION=$DEFAULT_GCC_VERSION
register_var_option "--gcc-version=<ver>" GCC_VERSION "Specify GCC version"

LLVM_VERSION=
register_var_option "--llvm-version=<ver>" LLVM_VERSION "Specify LLVM version"


register_jobs_option

extract_parameters "$@"

ABIS=$(commas_to_spaces $ABIS)

# Handle NDK_DIR
if [ -z "$NDK_DIR" ] ; then
    NDK_DIR=$ANDROID_NDK_ROOT
    log "Auto-config: --ndk-dir=$NDK_DIR"
else
    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: NDK directory does not exists: $NDK_DIR"
        exit 1
    fi
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    BUILD_DIR=$NDK_TMPDIR/build-libportable
else
    BUILD_DIR=$OPTION_BUILD_DIR
fi
mkdir -p "$BUILD_DIR"
fail_panic "Could not create build directory: $BUILD_DIR"

# Location of the libportable source tree base
LIBPORTABLE_SRCDIR_BASE=$ANDROID_NDK_ROOT/../development/ndk/$LIBPORTABLE_SUBDIR
CPUFEATURE_SRCDIR=$ANDROID_NDK_ROOT/sources/android/cpufeatures

# Compiler flags we want to use
LIBPORTABLE_CFLAGS="-fPIC -O2 -DANDROID -D__ANDROID__ -ffunction-sections"
LIBPORTABLE_CFLAGS=$LIBPORTABLE_CFLAGS" -I$LIBPORTABLE_SRCDIR_BASE/common/include -I$CPUFEATURE_SRCDIR -D__HOST__"
LIBPORTABLE_CXXFLAGS="-fno-exceptions -fno-rtti"
LIBPORTABLE_LDFLAGS=""

# If the --no-makefile flag is not used, we're going to put all build
# commands in a temporary Makefile that we will be able to invoke with
# -j$NUM_JOBS to build stuff in parallel.
#
if [ -z "$NO_MAKEFILE" ]; then
    MAKEFILE=$BUILD_DIR/Makefile
else
    MAKEFILE=
fi

# build_libportable_for_abi
# $1: ABI
# $2: build directory
# $3: (optional) installation directory
build_libportable_libs_for_abi ()
{
    local ARCH BINPREFIX
    local ABI=$1
    local BUILDDIR="$2"
    local DSTDIR="$3"
    local SRC OBJ OBJECTS CFLAGS CXXFLAGS

    mkdir -p "$BUILDDIR"

    # If the output directory is not specified, use default location
    if [ -z "$DSTDIR" ]; then
        DSTDIR=$NDK_DIR/$LIBPORTABLE_SUBDIR/libs/$ABI
    fi

    mkdir -p "$DSTDIR"

    builder_begin_android $ABI "$BUILDDIR" "$GCC_VERSION" "$LLVM_VERSION" "$MAKEFILE"
    builder_set_srcdir "$LIBPORTABLE_SRCDIR"
    builder_set_dstdir "$DSTDIR"

    if [ -z "$VISIBLE_LIBLIBPORTABLE_STATIC" ]; then
        # No -fvisibility-inlines-hidden because it is for C++, and there is
        # no C++ code in libportable
        builder_cflags "$LIBPORTABLE_CFLAGS" # ToDo: -fvisibility=hidden
    else
        builder_cflags "$LIBPORTABLE_CFLAGS"
    fi
    builder_ldflags "$LIBPORTABLE_LDFLAGS"
    builder_sources $LIBPORTABLE_SOURCES

    builder_set_srcdir "$CPUFEATURE_SRCDIR"
    builder_sources $CPUFEATURE_SOURCES

    log "Building $DSTDIR/libportable.a"
    builder_static_library libportable

    builder_end
  # Extract __wrap functions and create a *.wrap file of "--wrap=symbol".
  # This file will be passed to g++ doing the link in
  #
  #    g++ -Wl,@/path/to/libportable.wrap
  #
    nm -a $DSTDIR/libportable.a | grep __wrap_ | awk '{print $3}' | sed '/^$/d' | \
        sed 's/_wrap_/|/' | awk -F'|' '{print $2}' | sort | uniq | \
        awk '{printf "--wrap=%s\n",$1}' > "$DSTDIR/libportable.wrap"
}

for ABI in $ABIS; do
    # List of sources to compile
    ARCH=$(convert_abi_to_arch $ABI)
    LIBPORTABLE_SRCDIR=$LIBPORTABLE_SRCDIR_BASE/arch-$ARCH
    LIBPORTABLE_SOURCES=$(cd $LIBPORTABLE_SRCDIR && ls *.[cS])
    CPUFEATURE_SOURCES=$(cd $CPUFEATURE_SRCDIR && ls *.[cS])
    build_libportable_libs_for_abi $ABI "$BUILD_DIR/$ABI" "$OUT_DIR"
done

# If needed, package files into tarballs
if [ -n "$PACKAGE_DIR" ] ; then
    for ABI in $ABIS; do
        FILES="$LIBPORTABLE_SUBDIR/libs/$ABI/libportable.*"
        PACKAGE="$PACKAGE_DIR/libportable-libs-$ABI.tar.bz2"
        log "Packaging: $PACKAGE"
        pack_archive "$PACKAGE" "$NDK_DIR" "$FILES"
        fail_panic "Could not package $ABI libportable binaries!"
        dump "Packaging: $PACKAGE"
    done
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    log "Cleaning up..."
    rm -rf $BUILD_DIR
else
    log "Don't forget to cleanup: $BUILD_DIR"
fi

log "Done!"
