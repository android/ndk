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
#  This shell script is used to download the sources of the ccache
#  tool that can be used to speed-up rebuilds of NDK binaries.
#
#  We use a special patched version of ccache 2.4 that works
#  well on Win32 and handles the dependency generation compiler
#  flags (-MMD -MP -MF) properly.
#
#  Beta versions of ccache 3.0 are supposed to do that as well but
#  have not been checked yet.
#

# include common function and variable definitions
. `dirname $0`/../core/ndk-common.sh

print_help() {
    echo "Rebuild the prebuilt ccache binary for the Android NDK toolchain."
    echo ""
    echo "This script will automatically download the sources from the"
    echo "Internet, unless you use the --package=<file> option to specify"
    echo "the exact source package to use."
    echo ""
    echo "options (defaults are within brackets):"
    echo ""
    echo "  --help                   print this message"
    echo "  --package=<file>         specify download source package"
    echo "  --build-out=<path>       set temporary build out directory [/tmp/<random>]"
    echo "  --out=<file>             set output file for binary tarball"
    echo "  --install                install into NDK prebuilt directory"
    echo ""
}

BUILD_OUT=`mktemp -d /tmp/ndk-toolchain-XXX`

OPTION_PACKAGE=
OPTION_BUILD_OUT=
OPTION_OUT=
OPTION_INSTALL=no

VERBOSE=no
for opt do
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case "$opt" in
    --help|-h|-\?) OPTION_HELP=yes
        ;;
    --verbose)
        VERBOSE=yes
        ;;
    --package=*)
        OPTION_PACKAGE="$optarg"
        ;;
    --build-out=*)
        OPTION_BUILD_OUT="$optarg"
        ;;
    --out=*)
        OPTION_OUT="$optarg"
        ;;
    --install)
        OPTION_INSTALL=yes
        ;;
    *)
        echo "unknown option '$opt', use --help"
        exit 1
    esac
done

if [ "$OPTION_HELP" = "yes" ] ; then
    print_help
    exit 0
fi

# Force generation of 32-bit binaries on 64-bit systems
case $HOST_TAG in
    *-x86_64)
        HOST_CFLAGS="$HOST_CFLAGS -m32"
        HOST_LDFLAGS="$HOST_LDFLAGS -m32"
        force_32bit_binaries  # to modify HOST_TAG and others
        ;;
esac

setup_log_file

if [ -n "$OPTION_BUILD_OUT" ] ; then
    BUILD_OUT=$OPTION_BUILD_OUT
    log "Using specific build out directory: $BUILD_OUT"
else
    log "Using default random build out directory: $BUILD_OUT"
fi


# Where all generated files will be placed
OUT=$BUILD_OUT

# Check for md5sum
check_md5sum

# And wget/curl/scp too
find_program WGET wget
find_program CURL curl
find_program SCP scp

run rm -rf $BUILD_OUT
run mkdir -p $BUILD_OUT

if [ -n "$OPTION_PACKAGE" ] ; then
    if [ ! -f "$OPTION_PACKAGE" ] ; then
        dump "Your --package does not point to a valid file !"
        exit 1
    fi
    tar tzf $OPTION_PACKAGE > /dev/null 2>&1
    if [ $? != 0 ] ; then
        dump "It looks like your --package does not point to well-formed .tar.gz archive !"
        exit 1
    fi
    CCACHE_PACKAGE=$OPTION_PACKAGE
    CCACHE_VERSION=`echo $CCACHE_PACKAGE | sed s/\.tar\.gz//g`
    CCACHE_VERSION=`basename $CCACHE_VERSION`
else
    CCACHE_VERSION=ccache-2.4-android-20070905
    CCACHE_PACKAGE=$CCACHE_VERSION.tar.gz
    DOWNLOAD_ROOT=http://android.git.kernel.org/pub
fi

dump "Getting sources from $DOWNLOAD_ROOT/$CCACHE_PACKAGE"

download_file $DOWNLOAD_ROOT/$CCACHE_PACKAGE $BUILD_OUT/$CCACHE_PACKAGE
if [ $? != 0 ] ; then
    dump "Could not download $DOWNLOAD_ROOT/$CCACHE_PACKAGE"
    dump "Aborting."
    exit 1
fi

cd $BUILD_OUT && tar xzf $BUILD_OUT/$CCACHE_PACKAGE
if [ $? != 0 ] ; then
    dump "Could not unpack $CCACHE_PACKAGE in $BUILD_OUT"
    exit 1
fi

echo "Building ccache from sources..."
cd $BUILD_OUT/$CCACHE_VERSION && run make clean && run make unpack && run make build
if [ $? != 0 ] ; then
    dump "Could not build ccache in $BUILD_OUT"
fi

PREBUILT_DIR=prebuilt/$HOST_TAG/ccache

dump "Packaging ccache binary tarball..."
if [ -n "$OPTION_OUT" ] ; then
    CCACHE_TARBALL="$OPTION_OUT"
else
    CCACHE_TARBALL=/tmp/ccache-$HOST_TAG.tar.bz2
fi
cd $BUILD_OUT && run mkdir -p $PREBUILT_DIR && mv $CCACHE_VERSION/ccache $PREBUILT_DIR && run chmod a+x $PREBUILT_DIR/ccache && run tar cjf $CCACHE_TARBALL $PREBUILT_DIR/ccache
if [ $? != 0 ] ; then
    dump "Could not package ccache binary !"
    exit 1
fi
echo "Ccache binary tarball is ready in $CCACHE_TARBALL"

if [ $OPTION_INSTALL = yes ] ; then
    dump "Installing directly into $ANDROID_NDK_ROOT/$PREBUILT_DIR/ccache"
    run mkdir -p $ANDROID_NDK_ROOT/$PREBUILT_DIR
    if [ $? != 0 ] ; then
        dump "Could not install ccache binary to $PREBUILT_DIR"
        exit 1
    fi
    run cp -f $BUILD_OUT/$CCACHE_VERSION/ccache $ANDROID_NDK_ROOT/$PREBUILT_DIR/ccache
    # cleanup if needed
    if [ -n "$OPTION_BUILD_OUT" ] ; then
        rm -rf $BUILD_OUT
    fi
fi

dump "Done"
