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

# Download and rebuild all prebuilt binaries for the Android NDK
# This includes:
#    - cross toolchains (gcc, ld, gdb, etc...)
#    - target-specific gdbserver
#    - host ccache
#

. `dirname $0`/prebuilt-common.sh
PROGDIR=`dirname $0`

prepare_host_flags

PROGRAM_PARAMETERS=
PROGRAM_DESCRIPTION=\
"Download and rebuild from scratch all prebuilt binaries for the Android NDK."

NDK_DIR=$ANDROID_NDK_ROOT

register_option "--ndk-dir=<path>" do_ndk_dir "Specify target NDK directory" "$NDK_DIR"
do_ndk_dir () { OPTION_NDK_DIR=$1; }

BUILD_DIR=`random_temp_directory`
OPTION_BUILD_DIR=
register_option "--build-dir=<path>" do_build_dir "Specify temporary build directory" "/tmp/<random>"
do_build_dir () { OPTION_BUILD_DIR=$1; }

GDB_VERSION=6.6
OPTION_GDB_VERSION=
register_option "--gdb-version=<version>" do_gdb_version "Specify gdb version" "$GDB_VERSION"
do_gdb_version () { OPTION_GDB_VERSION=$1; }

OPTION_TOOLCHAIN_SRC_PKG=
OPTION_TOOLCHAIN_SRC_DIR=
register_option "--toolchain-src-pkg=<file>" do_toolchain_src_pkg "Use toolchain source package."
register_option "--toolchain-src-dir=<path>" do_toolchain_src_dir "Use toolchain source directory."
do_toolchain_src_pkg () { OPTION_TOOLCHAIN_SRC_PKG=$1; }
do_toolchain_src_dir () { OPTION_TOOLCHAIN_SRC_DIR=$1; }

OPTION_PACKAGE=no
register_option "--package" do_package "Package prebuilt binaries."
do_package () { OPTION_PACKAGE=yes; }

OPTION_TRY_X86=no
register_option "--try-x86" do_try_x86 "Build experimental x86 toolchain too."
do_try_x86 () { OPTION_TRY_X86=yes; }

extract_parameters $@

if [ "$OPTION_PACKAGE" = yes -a -z "$OPTION_NDK_DIR" ] ; then
    NDK_DIR=/tmp/ndk-prebuilt-$$
fi

if [ -n "$PARAMETERS" ]; then
    dump "ERROR: Too many parameters. See --help for proper usage."
    exit 1
fi

fix_option NDK_DIR "$OPTION_NDK_DIR" "target NDK directory"
fix_option BUILD_DIR "$OPTION_BUILD_DIR" "build directory"
fix_option GDB_VERSION "$OPTION_GDB_VERSION" "gdb version"

mkdir -p $BUILD_DIR
if [ $? != 0 ] ; then
    dump "ERROR: Could not create build directory: $BUILD_DIR"
    exit 1
fi

if [ -n "$OPTION_TOOLCHAIN_SRC_DIR" ] ; then
    if [ -n "$OPTION_TOOLCHAIN_SRC_PKG" ] ; then
        dump "ERROR: You can't use both --toolchain-src-dir and --toolchain-src-pkg"
        exi t1
    fi
    SRC_DIR=$OPTION_TOOLCHAIN_SRC_DIR
    if [ ! -d $SRC_DIR/gcc ] ; then
        dump "ERROR: Invalid toolchain source directory: $SRC_DIR"
        exit 1
    fi
else
    SRC_DIR=$BUILD_DIR/src
    mkdir -p $SRC_DIR
fi

FLAGS=""
if [ $VERBOSE = yes ] ; then
    FLAGS="--verbose"
fi

if [ -z "$OPTION_TOOLCHAIN_SRC_DIR" ] ; then
    if [ -n "$OPTION_TOOLCHAIN_SRC_PKG" ] ; then
        # Unpack the toolchain sources
        if [ ! -f "$OPTION_TOOLCHAIN_SRC_PKG" ] ; then
            dump "ERROR: Invalid toolchain source package: $OPTION_TOOLCHAIN_SRC_PKG"
            exit 1
        fi
        TARFLAGS="xf"
        if [ $VERBOSE2 = yes ] ; then
            TARFLAGS="v$TARFLAGS"
        fi
        if pattern_match '\.tar\.gz$' "$OPTION_TOOLCHAIN_SRC_PKG"; then
            TARFLAGS="z$TARFLAGS"
        fi
        if pattern_match '\.tar\.bz2$' "$OPTION_TOOLCHAIN_SRC_PKG"; then
            TARFLAGS="j$TARFLAGS"
        fi
        dump "Unpack sources from $OPTION_TOOLCHAIN_SRC_PKG"
        mkdir -p $SRC_DIR && tar $TARFLAGS $OPTION_TOOLCHAIN_SRC_PKG -C $SRC_DIR
        if [ $? != 0 ] ; then
            dump "ERROR: Could not unpack toolchain sources!"
            exit 1
        fi
    else
        # Download the toolchain sources
        dump "Download sources from android.git.kernel.org"
        $PROGDIR/download-toolchain-sources.sh $FLAGS $SRC_DIR
        if [ $? != 0 ] ; then
            dump "ERROR: Could not download toolchain sources!"
            exit 1
        fi
    fi

    # Patch the sources
    PATCHES_DIR=$PROGDIR/patches
    if [ -d "$PATCHES_DIR" ] ; then
        $PROGDIR/patch-sources.sh $FLAGS $SRC_DIR $PATCHES_DIR
        if [ $? != 0 ] ; then
            dump "ERROR: Could not patch sources."
            exit 1
        fi
    fi

fi # ! $TOOLCHAIN_SRC_DIR

# Build the toolchain from sources
build_toolchain ()
{
    dump "Building $1 toolchain... (this can be long)"
    $PROGDIR/build-gcc.sh $FLAGS --build-out=$BUILD_DIR/toolchain-$1 $SRC_DIR $NDK_DIR $1
    if [ $? != 0 ] ; then
        dump "ERROR: Could not build $1 toolchain!"
        exit 1
    fi
}

build_gdbserver ()
{
    dump "Build $1 gdbserver..."
    $PROGDIR/build-gdbserver.sh $FLAGS --build-out=$BUILD_DIR/gdbserver-$1 $SRC_DIR/gdb/gdb-$GDB_VERSION/gdb/gdbserver $NDK_DIR $1
    if [ $? != 0 ] ; then
        dump "ERROR: Could not build $1 toolchain!"
        exit 1
    fi
}

build_toolchain arm-eabi-4.2.1
build_gdbserver arm-eabi-4.2.1

build_toolchain arm-eabi-4.4.0
build_gdbserver arm-eabi-4.4.0

if [ "$OPTION_TRY_X86" = "yes" ] ; then
    build_toolchain x86-4.2.1
    build_gdbserver x86-4.2.1
fi

# XXX: NOT YET NEEDED!
#
#dump "Building host ccache binary..."
#$PROGDIR/build-ccache.sh $FLAGS --build-out=$BUILD_DIR/ccache $NDK_DIR
#if [ $? != 0 ] ; then
#    dump "ERROR: Could not build host ccache binary!"
#    exit 1
#fi

if [ "$OPTION_PACKAGE" = yes ] ; then
    RELEASE=`date +%Y%m%d`
    dump "Packaging prebuilt binaries..."
    PREBUILT_PACKAGE=/tmp/android-ndk-prebuilt-$RELEASE-$HOST_TAG.tar.bz2
    cd $NDK_DIR && tar cjf $PREBUILT_PACKAGE *
    if [ $? != 0 ] ; then
        dump "ERROR: Could not package prebuilt binaries!"
        exit 1
    fi
    dump "See: $PREBUILT_PACKAGE"
fi

dump "Done!"
