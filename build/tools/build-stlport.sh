#!/bin/sh
#
# Copyright (C) 2010 The Android Open Source Project
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
#  This shell script is used to rebuild the prebuilt STLport binaries from
#  their sources. It requires a working NDK installation.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

# Location of the  test project we use to force the rebuild.
# This is relative to the current NDK directory.
PROJECT_SUBDIR=tests/build/prebuild-stlport

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Rebuild the prebuilt STLport binaries for the Android NDK.

This script is called when packaging a new NDK release. It will simply
rebuild the STLport static and shared libraries from sources by using
the dummy project under $PROJECT_SUBDIR and a valid NDK installation.

By default, this will try with the current NDK directory, unless
you use the --ndk-dir=<path> option.

The output will be placed in appropriate sub-directories of
<ndk>/$STLPORT_SUBDIR, but you can override this with the --out-dir=<path>
option.
"

RELEASE=`date +%Y%m%d`
PACKAGE_DIR=/tmp/ndk-$USER/prebuilt/prebuilt-$RELEASE
register_var_option "--package-dir=<path>" PACKAGE_DIR "Put prebuilt tarballs into <path>."

NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "Specify NDK root path for the build."

BUILD_DIR=
OPTION_BUILD_DIR=
register_var_option "--build-dir=<path>" OPTION_BUILD_DIR "Specify temporary build dir."

OUT_DIR=
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory directly."

ABIS="$STLPORT_ABIS"
register_var_option "--abis=<list>" ABIS "Specify list of target ABIs."

JOBS="$BUILD_NUM_CPUS"
register_var_option "-j<number>" JOBS "Use <number> build jobs in parallel"

extract_parameters "$@"

ABIS=$(commas_to_spaces $ABIS)

# Handle NDK_DIR
if [ -z "$NDK_DIR" ] ; then
    NDK_DIR=$ANDROID_NDK_ROOT
    echo "Auto-config: --ndk-dir=$NDK_DIR"
else
    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: NDK directory does not exists: $NDK_DIR"
        exit 1
    fi
fi

# Determine target architectures from the ABI list
ARCHS=
for ABI in $ABIS; do
    case $ABI in
        armeabi|armeabi-v7a)
            ARCHS=$ARCHS" arm"
            ;;
        x86)
            ARCHS=$ARCHS" x86"
            ;;
        *)
            echo "ERROR: Unsupported ABI: $ABI"
            exit 1
    esac
done
ARCHS=$(sort_uniq $ARCHS)
log "Building for archs: $ARCHS"

# Sanity check the NDK directory
log "Checking NDK: ndk-build"
if [ ! -f "$NDK_DIR/ndk-build" ]; then
    echo "ERROR: NDK directory is missing ndk-build: $NDK_DIR"
    exit 1
fi
for ARCH in $ARCHS; do
    log "Checking NDK: $ARCH sysroot"
    PLATFORM_SYSROOT=$(get_default_platform_sysroot_for_arch $ARCH)
    if [ ! -d "$NDK_DIR/$PLATFORM_SYSROOT" ]; then
        echo "ERROR: NDK directory is missing $PLATFORM_SYSROOT: $NDK_DIR"
        exit 1
    fi
    TOOLCHAIN_BINPREFIX=$(get_default_toolchain_binprefix_for_arch $ARCH)
    log "Checking NDK: $ARCH toolchain: $TOOLCHAIN_BINPREFIX"
    if [ ! -f "$NDK_DIR/${TOOLCHAIN_BINPREFIX}gcc" ]; then
        echo "ERROR: NDK directory is missing $(basename $TOOLCHAIN_BINPREFIX)gcc: $NDK_DIR"
        exit 1
    fi
done
log "Checking NDK: $SUBPROJECT_DIR"
if [ ! -d "$NDK_DIR/$SUBPROJECT_DIR" ]; then
    echo "ERROR: NDK directory is missing project $SUBPROJECT_DIR: $NDK_DIR"
    exit 1
fi


# Handle build directory
BUILD_DIR="$OPTION_BUILD_DIR"
if [ -n "$BUILD_DIR" ] ; then
    log "Using temporary build dir: $BUILD_DIR"
else
    BUILD_DIR=$NDK_TMPDIR/build-stlport
    log "Using random build dir: $BUILD_DIR"
fi
rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"

# Copy the project directory
dump "Copying NDK files to build directory."
run mkdir -p "$BUILD_DIR/$PROJECT_SUBDIR" &&
run cp -r "$NDK_DIR/$PROJECT_SUBDIR/*" "$BUILD_DIR/$PROJECT_SUBDIR/"
if [ $? != 0 ]; then
    dump "ERROR: Could not copy NDK sources to build directory!"
    exit 1
fi

#
# Setup our paths
#

if [ -z "$OUT_DIR" ] ; then
    OUT_DIR=$NDK_DIR/$STLPORT_SUBDIR
    log "Using default output dir: $OUT_DIR"
else
    log "Using user output dir: $OUT_DIR"
fi

#
# Now build the fake project
#
# NOTE: We take the build project from this NDK's tree, not from
#        the alternative one specified with --ndk=<dir>
#
PROJECT_DIR=$BUILD_DIR/$PROJECT_SUBDIR
if [ ! -d $PROJECT_DIR ] ; then
    dump "ERROR: Missing required project: $PROJECT_SUBDIR"
    exit 1
fi

# cleanup required to avoid problems with stale dependency files
rm -rf "$PROJECT_DIR/libs"
rm -rf "$PROJECT_DIR/obj"

LIBRARIES="libstlport_static.a libstlport_shared.so"

for ABI in $ABIS; do
    dump "Building $ABI STLport binaries..."
    ARCH=$(convert_abi_to_arch $ABI)
    PLATFORM=android-$(get_default_api_level_for_arch $ARCH)
    (run cd "$BUILD_DIR/$PROJECT_SUBDIR" && run "$NDK_DIR"/ndk-build -B APP_PLATFORM=$PLATFORM APP_ABI=$ABI -j$JOBS STLPORT_FORCE_REBUILD=true)
    if [ $? != 0 ] ; then
        dump "ERROR: Could not build $ABI STLport binaries!!"
        exit 1
    fi

    if [ -z "$PACKAGE_DIR" ] ; then
       # Copy files to target output directory
        SRCDIR="$PROJECT_DIR/obj/local/$ABI"
        DSTDIR="$OUT_DIR/libs/$ABI"
        copy_file_list "$SRCDIR" "$DSTDIR" "$LIBRARIES"
    fi
done

# If needed, package files into tarballs
if [ -n "$PACKAGE_DIR" ] ; then
    for ABI in $ABIS; do
        FILES=""
        for LIB in $LIBRARIES; do
            SRCDIR="$PROJECT_DIR/obj/local/$ABI"
            DSTDIR="$STLPORT_SUBDIR/libs/$ABI"
            copy_file_list "$SRCDIR" "$BUILD_DIR/$DSTDIR" "$LIB"
            log "Installing: $DSTDIR/$LIB"
            FILES="$FILES $DSTDIR/$LIB"
        done
        PACKAGE="$PACKAGE_DIR/stlport-libs-$ABI.tar.bz2"
        pack_archive "$PACKAGE" "$BUILD_DIR" "$FILES"
        fail_panic "Could not package $ABI STLport binaries!"
        dump "Packaging: $PACKAGE"
    done
fi

dump "Cleaning up..."
rm -rf $BUILD_DIR

dump "Done!"
