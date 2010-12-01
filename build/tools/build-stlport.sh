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
PACKAGE_DIR=/tmp/ndk-prebuilt/prebuilt-$RELEASE
register_var_option "--package-dir=<path>" PACKAGE_DIR "Put prebuilt tarballs into <path>."

NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "Don't package, put the files in target NDK dir."

TOOLCHAIN_PKG=
register_var_option "--toolchain-pkg=<path>" TOOLCHAIN_PKG "Use specific toolchain prebuilt package."

BUILD_DIR=
OPTION_BUILD_DIR=
register_var_option "--build-dir=<path>" OPTION_BUILD_DIR "Specify temporary build dir."

OUT_DIR=
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory directly."

ABIS="$STLPORT_ABIS"
register_var_option "--abis=<list>" ABIS "Specify list of target ABIs."

extract_parameters "$@"

if [ -n "$PACKAGE_DIR" -a -n "$NDK_DIR" ] ; then
    echo "ERROR: You cannot use both --package-dir and --ndk-dir at the same time!"
    exit 1
fi

if [ -n "$TOOLCHAIN_PKG" ] ; then
    if [ ! -f "$TOOLCHAIN_PKG" ] ; then
        dump "ERROR: Your toolchain package does not exist: $TOOLCHAIN_PKG"
        exit 1
    fi
    case "$TOOLCHAIN_PKG" in
        *.tar.bz2)
            ;;
        *)
            dump "ERROR: Toolchain package is not .tar.bz2 archive: $TOOLCHAIN_PKG"
            exit 1
    esac
fi

if [ -z "$NDK_DIR" ] ; then
    mkdir -p "$PACKAGE_DIR"
    if [ $? != 0 ] ; then
        echo "ERROR: Could not create directory: $PACKAGE_DIR"
        exit 1
    fi
    NDK_DIR=/tmp/ndk-toolchain/ndk-prebuilt-$$
    mkdir -p $NDK_DIR &&
    dump "Copying NDK files to temporary dir: $NDK_DIR"
    run cp -rf $ANDROID_NDK_ROOT/* $NDK_DIR/
    if [ -n "$TOOLCHAIN_PKG" ] ; then
        dump "Extracting prebuilt toolchain binaries."
        unpack_archive "$TOOLCHAIN_PKG" "$NDK_DIR"
    fi
else
    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: NDK directory does not exists: $NDK_DIR"
        exit 1
    fi
    PACKAGE_DIR=
fi

#
# Setup our paths
#
log "Using NDK root: $NDK_DIR"

BUILD_DIR="$OPTION_BUILD_DIR"
if [ -n "$BUILD_DIR" ] ; then
    log "Using temporary build dir: $BUILD_DIR"
else
    BUILD_DIR=`random_temp_directory`
    log "Using random build dir: $BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

if [ -z "$OUT_DIR" ] ; then
    OUT_DIR=$NDK_DIR/$STLPORT_SUBDIR
    log "Using default output dir: $OUT_DIR"
else
    log "Using usr output dir: $OUT_DIR"
fi

#
# Now build the fake project
#
# NOTE: We take the build project from this NDK's tree, not from
#        the alternative one specified with --ndk=<dir>
#
PROJECT_DIR="$ANDROID_NDK_ROOT/$PROJECT_SUBDIR"
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
    (run cd "$PROJECT_SUBDIR" && run "$NDK_DIR"/ndk-build -B APP_ABI=$ABI -j$BUILD_JOBS STLPORT_FORCE_REBUILD=true)
    if [ $? != 0 ] ; then
        dump "ERROR: Could not build $ABI STLport binaries!!"
        exit 1
    fi

    if [ -z "$PACKAGE_DIR" ] ; then
       # Copy files to target NDK
        SRCDIR="$PROJECT_SUBDIR/obj/local/$ABI"
        DSTDIR="$OUT_DIR/libs/$ABI"
        copy_file_list "$SRCDIR" "$DSTDIR" "$LIBRARIES"
    fi
done

# If needed, package files into tarballs
if [ -n "$PACKAGE_DIR" ] ; then
    for ABI in $ABIS; do
        FILES=""
        for LIB in $LIBRARIES; do
            SRCDIR="$PROJECT_SUBDIR/obj/local/$ABI"
            DSTDIR="$STLPORT_SUBDIR/libs/$ABI"
            copy_file_list "$SRCDIR" "$NDK_DIR/$DSTDIR" "$LIB"
            log "Installing: $DSTDIR/$LIB"
            FILES="$FILES $DSTDIR/$LIB"
        done
        PACKAGE="$PACKAGE_DIR/stlport-libs-$ABI.tar.bz2"
        pack_archive "$PACKAGE" "$NDK_DIR" "$FILES"
        fail_panic "Could not package $ABI STLport binaries!"
        dump "Packaging: $PACKAGE"
    done
fi

if [ -n "$PACKAGE_DIR" ] ; then
    dump "Cleaning up..."
    rm -rf $NDK_DIR
fi

dump "Done!"
