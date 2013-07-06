#!/bin/sh
#
# Copyright (C) 2012 The Android Open Source Project
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
# Rebuild the mingw64 cross-toolchain from scratch
#
# Sample (recommended for compatibility reasons) command line:
#
# GOOGLE_PREBUILT=<some folder>
# git clone https://android.googlesource.com/platform/prebuilt $GOOGLE_PREBUILT
# export PATH=$GOOGLE_PREBUILT/linux-x86/toolchain/i686-linux-glibc2.7-4.6/bin:$PATH
# build-mingw64-toolchain.sh --target-arch=i686                       \
#                            --package-dir=i686-w64-mingw32-toolchain \
#                            --binprefix=i686-linux
#

PROGNAME=$(basename $0)
PROGDIR=$(dirname $0)
PROGDIR=$(cd $PROGDIR && pwd)

HELP=
VERBOSE=1

# This will be reset later.
LOG_FILE=/dev/null

panic ()
{
    1>&2 echo "Error: $@"
    exit 1
}

fail_panic ()
{
    if [ $? != 0 ]; then
        panic "$@"
    fi
}

var_value ()
{
    eval echo \"$1\"
}

var_append ()
{
    local _varname=$1
    local _varval=$(var_value $_varname)
    shift
    if [ -z "$_varval" ]; then
        eval $_varname=\"$*\"
    else
        eval $_varname=\$$_varname\" $*\"
    fi
}

run ()
{
    if [ "$VERBOSE" -gt 0 ]; then
        echo "COMMAND: >>>> $@" >> $LOG_FILE
    fi
    if [ "$VERBOSE" -gt 1 ]; then
        echo "COMMAND: >>>> $@"
    fi
    if [ "$VERBOSE" -gt 1 ]; then
        "$@"
    else
       "$@" > /dev/null 2>&1
    fi
}

log ()
{
    if [ "$LOG_FILE" ]; then
        echo "$@" >> $LOG_FILE
    fi
    if [ "$VERBOSE" -gt 0 ]; then
        echo "$@"
    fi
}

# For now, only tested on Linux
OS=$(uname -s)
EXEEXT= # executable extension
case $OS in
    Linux) OS=linux;;
    Darwin) OS=darwin;;
    CYGWIN*|*_NT-*) OS=windows;
        if [ "$OSTYPE" = cygwgin ]; then
            OS=cygwin
        fi
        EXEEXT=.exe
        ;;
esac

ARCH=$(uname -m)
case $ARCH in
    i?86) ARCH=i686;;
    amd64) ARCH=x86_64;;
esac

case $OS in
    linux)
        NUM_CORES=$(grep -c -e '^processor' /proc/cpuinfo)
        ;;
    darwin|freebsd)
        NUM_CORES=`sysctl -n hw.ncpu`
        ;;
    windows|cygwin)
        NUM_CORES=$NUMBER_OF_PROCESSORS
        ;;
    *)  # let's play safe here
        NUM_CORES=1
        ;;
esac

# Warn our users, because the script probably fails on anything but Linux
# at that point (e.g. there are strange libtool build breakages on darwin).
if [ "$OS" != "linux" ]; then
    echo "WARNING: WARNING: WARNING: THIS SCRIPT PROBABLY ONLY WORKS ON LINUX!!"
fi

# GMP moving home?
# GMP_VERSION=5.1.0
# GMP_URL=ftp://ftp.gmplib.org/pub/gmp-$GMP_VERSION/
# ..but the old one is still there:
GMP_VERSION=5.0.5
GMP_URL=http://ftp.gnu.org/gnu/gmp/

MPFR_VERSION=3.1.1
MPC_VERSION=1.0.1
BINUTILS_VERSION=2.22
GCC_VERSION=4.7.2

# Need at least revision 5166
# For reference, I've built a working NDK with 5445
# (latest as of Sun Feb 3 2013 is 5578)
MINGW_W64_VERSION=svn@5861

JOBS=$(( $NUM_CORES * 2 ))


HOST_BINPREFIX=
TARGET_ARCH=x86_64
TARGET_MULTILIBS=true  # not empty to enable multilib
PACKAGE_DIR=
FORCE_ALL=
FORCE_BUILD=
CLEANUP=

TEMP_DIR=/tmp/build-mingw64-toolchain-$USER

for opt; do
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case $opt in
        -h|-?|--help) HELP=true;;
        --verbose) VERBOSE=$(( $VERBOSE + 1 ));;
        --quiet) VERBOSE=$(( $VERBOSE - 1 ));;
        --binprefix=*) HOST_BINPREFIX=$optarg;;
        -j*|--jobs=*) JOBS=$optarg;;
        --target-arch=*) TARGET_ARCH=$optarg;;
        --no-multilib) TARGET_MULTILIBS="";;
        --force-build) FORCE_BUILD=true;;
        --force-all) FORCE_ALL=true;;
        --work-dir=*) TEMP_DIR=$optarg;;
        --package-dir=*) PACKAGE_DIR=$optarg;;
        --cleanup) CLEANUP=true;;
        --gcc-version=*) GCC_VERSION=$optarg;;
        --binutils-version=*) BINUTILS_VERSION=$optarg;;
        --gmp-version=*) GMP_VERSION=$optarg;;
        --mpfr-version=*) MPFR_VERSION=$optarg;;
        --mpc-version=*) MPC_VERSION=$optarg;;
        --mingw-version=*) MINGW_W64_VERSION=$optarg;;
        -*) panic "Unknown option '$opt', see --help for list of valid ones.";;
        *) panic "This script doesn't take any parameter, see --help for details.";;
    esac
done


if [ "$HELP" ]; then
    echo "Usage: $PROGNAME [options]"
    echo ""
    echo "This program is used to rebuild a mingw64 cross-toolchain from scratch."
    echo ""
    echo "Valid options:"
    echo "  -h|-?|--help                 Print this message."
    echo "  --verbose                    Increase verbosity."
    echo "  --quiet                      Decrease verbosity."
    echo "  --gcc-version=<version>      Select gcc version [$GCC_VERSION]."
    echo "  --binutil-version=<version>  Select binutils version [$BINUTILS_VERSION]."
    echo "  --gmp-version=<version>      Select libgmp version [$GMP_VERSION]."
    echo "  --mpfr-version=<version>     Select libmpfr version [$MPFR_VERSION]."
    echo "  --mpc-version=<version>      Select libmpc version [$MPC_VERSION]."
    echo "  --mingw-version=<version>    Select mingw-w64 version [$MINGW_W64_VERSION]."
    echo "  --jobs=<num>                 Run <num> build tasks in parallel [$JOBS]."
    echo "  -j<num>                      Same as --jobs=<num>."
    echo "  --binprefix=<prefix>         Specify bin prefix for host toolchain."
    echo "  --no-multilib                Disable multilib toolchain build."
    echo "  --target-arch=<arch>         Select default target architecture [$TARGET_ARCH]."
    echo "  --force-all                  Redo everything from scratch."
    echo "  --force-build                Force a rebuild (keep sources)."
    echo "  --cleanup                    Remove all temp files after build."
    echo "  --work-dir=<path>            Specify work/build directory [$TEMP_DIR]."
    echo "  --package-dir=<path>         Package toolchain to directory."
    echo ""
    exit 0
fi

if [ "$CLEANUP" ]; then
    if [ -z "$PACKAGE_DIR" ]; then
        panic "You should only use --cleanup with --package-dir=<path> !".
    fi
fi

BUILD_TAG64=x86_64-linux-gnu
BUILD_TAG32=i686-linux-gnu

# We don't want debug executables
BUILD_CFLAGS="-O2 -fomit-frame-pointer -s"
BUILD_LDFLAGS=""

# On Darwin, we want to use the 10.4 / 10.5 / 10.6 SDKs to generate binaries
# that work on "old" platform releases.
if [ "$OS" = darwin ]; then
    # Use the check for the availability of a compatibility SDK in Darwin
    # this can be used to generate binaries compatible with either Tiger or
    # Leopard.
    #
    # $1: SDK root path
    # $2: MacOS X minimum version (e.g. 10.4)
    check_darwin_sdk ()
    {
        if [ -d "$1" ] ; then
            var_append BUILD_CFLAGS "-isysroot $1 -mmacosx-version-min=$2 -DMAXOSX_DEPLOYEMENT_TARGET=$2"
            var_append BUILD_LDFLAGS "-Wl,-syslibroot,$sdk -mmacosx-version-min=$2"
            return 0  # success
        fi
        return 1
    }

    if check_darwin_sdk /Developer/SDKs/MacOSX10.4.sdku 10.4; then
        log "Generating Tiger-compatible binaries!"
    elif check_darwin_sdk /Developer/SDKs/MacOSX10.5.sdk 10.5; then
        log "Generating Leopard-compatible binaries!"
    elif check_darwin_sdk /Developer/SDKs/MacOSX10.6.sdk 10.6; then
        log "Generating Snow Leopard-compatible binaries!"
    else
        osx_version=`sw_vers -productVersion`
        log "Generating $osx_version-compatible binaries!"
    fi
fi

mkdir -p $TEMP_DIR
if [ "$FORCE_ALL" ]; then
    log "Cleaning up work directory..."
    rm -rf $TEMP_DIR/*
fi

LOG_FILE=$TEMP_DIR/build.log
rm -f $LOG_FILE && touch $LOG_FILE
if [ "$VERBOSE" -eq 1 ]; then
    echo  "To follow build, use in another terminal: tail -F $LOG_FILE"
fi

case $TARGET_ARCH in
    x86_64) TARGET_BITS=64;;
    i686) TARGET_BITS=32;;
    *) panic "Invalid --target parameter. Valid values are: x86_64 i686";;
esac
TARGET_TAG=$TARGET_ARCH-w64-mingw32
log "Target arch: $TARGET_TAG"
log "Target bits: $TARGET_BITS"

# Determine bitness of host architecture
PROBE_CC=${CC:-gcc}
if [ -n "$HOST_BINPREFIX" ]; then
    # If $HOST_BINPREFIX is a directory but not ends with '/', append '/'.
    # Otherwise, append '-'.
    if [ -d "$HOST_BINPREFIX" ] ; then
        if [ -n "${HOST_BINPREFIX##*/}" ] ; then
	    HOST_BINPREFIX="${HOST_BINPREFIX}/"
	fi
    else
        HOST_BINPREFIX="${HOST_BINPREFIX}-"
    fi
    PROBE_CC=${HOST_BINPREFIX}gcc
fi
echo "Using GCC: $PROBE_CC"
echo "int main() { return 0; }" > $TEMP_DIR/test-host-cc.c
$PROBE_CC -c $TEMP_DIR/test-host-cc.c -o $TEMP_DIR/test-host-cc.o > /dev/null
fail_panic "Host compiler doesn't work: $PROBE_CC"

file $TEMP_DIR/test-host-cc.o | grep -q -e "x86[_-]64"
if [ $? != 0 ]; then
    log "Host compiler generates 32-bit code: $PROBE_CC"
    HOST_ARCH=i686
    HOST_BITS=32
else
    log "Host compiler generates 64-bit code: $PROBE_CC"
    HOST_ARCH=x86_64
    HOST_BITS=64
fi

case $OS in
    linux) HOST_TAG=$HOST_ARCH-linux-gnu;;
    darwin) HOST_TAG=$HOST_ARCH-apple-darwinx11;;
    cygwin) HOST_TAG=$HOST_ARCH-pc-cygwin;;
    *) panic "Unsupported host operating system!"
esac
log "Host arch: $HOST_TAG"

download_package ()
{
    # Assume the packages are already downloaded under $ARCHIVE_DIR
    local PKG_URL=$1
    local PKG_NAME=$(basename $PKG_URL)

    case $PKG_NAME in
        *.tar.bz2)
            PKG_BASENAME=${PKG_NAME%%.tar.bz2}
            ;;
        *.tar.gz)
            PKG_BASENAME=${PKG_NAME%%.tar.gz}
            ;;
        *)
            panic "Unknown archive type: $PKG_NAME"
    esac

    if [ ! -f "$ARCHIVE_DIR/$PKG_NAME" ]; then
        log "Downloading $PKG_URL..."
        (cd $ARCHIVE_DIR && run curl -L -o "$PKG_NAME" "$PKG_URL")
        fail_panic "Can't download '$PKG_URL'"
    fi

    MD5SUM=$(md5sum $ARCHIVE_DIR/$PKG_NAME | cut -d" " -f1)
    echo "$MD5SUM  $PKG_URL" >> $INSTALL_DIR/README

    if [ ! -d "$SRC_DIR/$PKG_BASENAME" ]; then
        log "Uncompressing $PKG_URL into $SRC_DIR"
        case $PKG_NAME in
            *.tar.bz2)
                run tar xjf $ARCHIVE_DIR/$PKG_NAME -C $SRC_DIR
                ;;
            *.tar.gz)
                run tar xzf $ARCHIVE_DIR/$PKG_NAME -C $SRC_DIR
                ;;
            *)
                panic "Unknown archive type: $PKG_NAME"
                ;;
        esac
        fail_panic "Can't uncompress $ARCHIVE_DIR/$PKG_NAME"
    fi
}

# Download and unpack source packages from official sites
ARCHIVE_DIR=$TEMP_DIR/archive
SRC_DIR=$TEMP_DIR/src
STAMP_DIR=$TEMP_DIR/timestamps

mkdir -p $ARCHIVE_DIR
mkdir -p $SRC_DIR
mkdir -p $STAMP_DIR

INSTALL_DIR=$TEMP_DIR/install-$HOST_TAG/$TARGET_TAG
BUILD_DIR=$TEMP_DIR/build-$HOST_TAG

if [ "$FORCE_BUILD" ]; then
    rm -f $STAMP_DIR/*
    rm -rf $INSTALL_DIR
    rm -rf $BUILD_DIR
fi

# Make temp install directory
mkdir -p $INSTALL_DIR
mkdir -p $BUILD_DIR

# Copy this script
cp $0 $INSTALL_DIR/ &&
echo "This file has been automatically generated on $(date) with the following command:" > $INSTALL_DIR/README &&
echo "$PROGNAME $@" >> $INSTALL_DIR/README &&
echo "" >> $INSTALL_DIR/README &&
echo "The MD5 hashes for the original sources packages are:" >> $INSTALL_DIR/README
fail_panic "Could not copy script to installation directory."

download_package ${GMP_URL}gmp-${GMP_VERSION}.tar.bz2
download_package http://ftp.gnu.org/gnu/mpfr/mpfr-$MPFR_VERSION.tar.bz2
download_package http://www.multiprecision.org/mpc/download/mpc-$MPC_VERSION.tar.gz
download_package http://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.bz2
download_package http://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.bz2

PREFIX_FOR_TARGET=$INSTALL_DIR/$TARGET_TAG
WITH_WIDL=$INSTALL_DIR/bin
MINGW_W64_REVISION=
MINGW_W64_VERSION_NO_REV=$(echo $MINGW_W64_VERSION | awk 'BEGIN { FS="@" }; { print $1 }')
if [ "$MINGW_W64_VERSION_NO_REV" = "svn" ];  then
    MINGW_W64_REVISION=$(echo $MINGW_W64_VERSION | awk 'BEGIN { FS="@" }; { print $2 }')
    if [ ! -z "$MINGW_W64_REVISION" ] ; then
        if [ $MINGW_W64_REVISION -lt 5186 ] ; then
            PREFIX_FOR_TARGET=$INSTALL_DIR
        fi
        if [ $MINGW_W64_REVISION -lt 5252 ] ; then
            WITH_WIDL=mingw-w64-widl
        elif [ $MINGW_W64_REVISION -lt 5258 ] ; then
            WITH_WIDL=$TARGET_TAG-widl
        fi
        MINGW_W64_REVISION2=-r$MINGW_W64_REVISION
        MINGW_W64_REVISION=@${MINGW_W64_REVISION}
    fi
    MINGW_W64_SRC=$SRC_DIR/mingw-w64-svn$MINGW_W64_REVISION2
    MINGW_W64_VERSION=svn
fi

if [ -z "$MINGW_W64_REVISION" ] ; then
    # Released versions of MinGW-w64 don't provide easily accessible information
    # about the svn revision which this script needs to know.
    fail_panic "Building MinGW-w64 toolchain requires specifying an svn version"
fi

if [ ! -d $MINGW_W64_SRC ]; then
    echo "Checking out https://mingw-w64.svn.sourceforge.net/svnroot/mingw-w64/trunk$MINGW_W64_REVISION $MINGW_W64_SRC"
    run svn co https://mingw-w64.svn.sourceforge.net/svnroot/mingw-w64/trunk$MINGW_W64_REVISION $MINGW_W64_SRC
    PATCHES_DIR="$PROGDIR/toolchain-patches-host/mingw-w64"
    if [ -d "$PATCHES_DIR" ] ; then
        PATCHES=$(find "$PATCHES_DIR" -name "*.patch" | sort)
        for PATCH in $PATCHES; do
            echo "Patching mingw-w64-$MINGW_W64_REVISION with $PATCH"
            (cd $MINGW_W64_SRC && run patch -p0 < $PATCH)
        done
    fi
fi

# Let's generate the licenses/ directory
LICENSES_DIR=$INSTALL_DIR/licenses/
mkdir -p $LICENSES_DIR
if [ ! -f $STAMP_DIR/licenses ]; then
    LICENSE_FILES=$(cd $SRC_DIR && find . -name "COPYING*")
    # Copy all license files to $LICENSES_DIR
    (tar cf - -C $SRC_DIR $LICENSE_FILES) | (tar xf - -C $LICENSES_DIR)
    touch $STAMP_DIR/licenses
fi

setup_build_env ()
{
    local BINPREFIX=$1

    if [ "$BINPREFIX" ]; then
        CC=${BINPREFIX}gcc
        CXX=${BINPREFIX}g++
        LD=${BINPREFIX}ld
        AS=${BINPREFIX}as
        AR=${BINPREFIX}ar
        RANLIB=${BINPREFIX}ranlib
        STRIP=${BINPREFIX}strip
        export CC CXX LD AS AR RANLIB STRIP
    elif [ "$OS" = darwin ]; then
        # Needed on OS X otherwise libtool will try to use gcc and $BUILD_CFLAGS
        LD=ld
    fi

    export CFLAGS="$BUILD_CFLAGS"
    export CXXFLAGS="$BUILD_CFLAGS"
    export LDFLAGS="$BUILD_LDFLAGS"
}

setup_install_env ()
{
    export PATH=$INSTALL_DIR/bin:$PATH
}

build_host_package ()
{
    local PKGNAME=$1
    shift

    if [ ! -f $STAMP_DIR/$PKGNAME ]; then
        (
            mkdir -p $BUILD_DIR/$PKGNAME &&
            cd $BUILD_DIR/$PKGNAME &&
            setup_build_env $HOST_BINPREFIX &&
            log "$PKGNAME: Configuring" &&
            run $SRC_DIR/$PKGNAME/configure "$@"
            fail_panic "Can't configure $PKGNAME !!"

            log "$PKGNAME: Building" &&
            run make -j$JOBS
            fail_panic "Can't build $PKGNAME !!"

            log "$PKGNAME: Installing" &&
            run make install
            fail_panic "Can't install $PKGNAME"
        ) || exit 1
        touch $STAMP_DIR/$PKGNAME
    fi
}

export ABI=$HOST_BITS
BASE_HOST_OPTIONS="--prefix=$INSTALL_DIR --disable-shared"
build_host_package gmp-$GMP_VERSION $BASE_HOST_OPTIONS
var_append BASE_HOST_OPTIONS "--with-gmp=$INSTALL_DIR"

build_host_package mpfr-$MPFR_VERSION $BASE_HOST_OPTIONS
var_append BASE_HOST_OPTIONS "--with-mpfr=$INSTALL_DIR"

build_host_package mpc-$MPC_VERSION $BASE_HOST_OPTIONS
var_append BASE_HOST_OPTIONS "--with-mpc=$INSTALL_DIR"

BINUTILS_CONFIGURE_OPTIONS=$BASE_HOST_OPTIONS
var_append BINUTILS_CONFIGURE_OPTIONS "--target=$TARGET_TAG --disable-nls"
if [ "$TARGET_MULTILIBS" ]; then
    var_append BINUTILS_CONFIGURE_OPTIONS "--enable-targets=x86_64-w64-mingw32,i686-w64-mingw32"
fi

var_append BINUTILS_CONFIGURE_OPTIONS "--with-sysroot=$INSTALL_DIR"

build_host_package binutils-$BINUTILS_VERSION $BINUTILS_CONFIGURE_OPTIONS

build_mingw_tools ()
{
    local PKGNAME=$1
    echo "$STAMP_DIR/$PKGNAME"
    if [ ! -f "$STAMP_DIR/$PKGNAME" ]; then
        (
            mkdir -p $BUILD_DIR/$PKGNAME &&
            cd $BUILD_DIR/$PKGNAME &&
            log "$PKGNAME: Configuring" &&
            run $MINGW_W64_SRC/mingw-w64-tools/widl/configure --prefix=$INSTALL_DIR --target=$TARGET_TAG
            fail_panic "Can't configure mingw-64-tools"
            log "$PKGNAME: Installing" &&
            run make install -j$JOBS
        ) || exit 1
        touch $STAMP_DIR/$PKGNAME
    fi
}

# Install the right mingw64 headers into the sysroot
build_mingw_headers ()
{
    local PKGNAME=$1
    if [ ! -f "$STAMP_DIR/$PKGNAME" ]; then
        (
            # If --with-widl only identifies the program name (svn version dependent)...
            if [ $(basename "$WITH_WIDL") = "$WITH_WIDL" ] ; then
                # ...then need to add the right path too.
                export PATH=$PATH:$INSTALL_DIR/bin
            fi
            fail_panic "Can't find widl"
            mkdir -p $BUILD_DIR/$PKGNAME &&
            cd $BUILD_DIR/$PKGNAME &&
            log "$PKGNAME: Configuring" &&
            run $MINGW_W64_SRC/mingw-w64-headers/configure --prefix=$PREFIX_FOR_TARGET --host=$TARGET_TAG \
                --build=$HOST_TAG --with-widl=$WITH_WIDL --enable-sdk=all
            fail_panic "Can't configure mingw-64-headers"

            run make
            log "$PKGNAME: Installing" &&
            run make install -j$JOBS &&
            run cd $INSTALL_DIR && 
            run ln -s $TARGET_TAG mingw &&
            run cd $INSTALL_DIR/mingw && 
            run ln -s lib lib$TARGET_BITS
            fail_panic "Can't configure mingw-64-headers"
        ) || exit 1
        touch $STAMP_DIR/$PKGNAME
    fi
}

# Slightly different from build_host_package because we need to call
# 'make all-gcc' and 'make install-gcc' as a special case.
#
build_core_gcc ()
{
    local PKGNAME=$1
    shift

    if [ ! -f "$STAMP_DIR/core-$PKGNAME" ]; then
        (
            mkdir -p $BUILD_DIR/$PKGNAME &&
            cd $BUILD_DIR/$PKGNAME &&
            setup_build_env $HOST_BINPREFIX &&
            log "core-$PKGNAME: Configuring" &&
            run $SRC_DIR/$PKGNAME/configure "$@"
            fail_panic "Can't configure $PKGNAME !!"

            log "core-$PKGNAME: Building" &&
            run make -j$JOBS all-gcc
            fail_panic "Can't build $PKGNAME !!"

            log "core-$PKGNAME: Installing" &&
            run make -j$JOBS install-gcc
            fail_panic "Can't install $PKGNAME"
        ) || exit 1
        touch $STAMP_DIR/core-$PKGNAME
    fi
}


# Build and install the C runtime files needed by the toolchain
build_mingw_crt ()
{
    local PKGNAME=$1
    shift

    if [ ! -f $STAMP_DIR/$PKGNAME ]; then
        (
            mkdir -p $BUILD_DIR/$PKGNAME &&
            cd $BUILD_DIR/$PKGNAME &&
            export PATH=$INSTALL_DIR/bin:$PATH
            log "$PKGNAME: Configuring" &&
            run $MINGW_W64_SRC/mingw-w64-crt/configure "$@"
            fail_panic "Can't configure $PKGNAME !!"

            log "$PKGNAME: Building" &&
            run make -j$JOBS
            fail_panic "Can't build $PKGNAME !!"

            log "$PKGNAME: Installing" &&
            run make -j$JOBS install
            fail_panic "Can't install $PKGNAME"
        ) || exit 1
        touch $STAMP_DIR/$PKGNAME
    fi
}


build_libgcc ()
{
    local PKGNAME=$1
    shift

    if [ ! -f "$STAMP_DIR/libgcc-$PKGNAME" ]; then
        (
            # No configure step here! We're resuming work that was started
            # in build_core_gcc.
            cd $BUILD_DIR/$PKGNAME &&
            setup_build_env $HOST_BINPREFIX &&
            log "libgcc-$PKGNAME: Building" &&
            run make -j$JOBS
            fail_panic "Can't build libgcc-$PKGNAME !!"

            log "libgcc-$PKGNAME: Installing" &&
            run make -j$JOBS install
            fail_panic "Can't install libgcc-$PKGNAME"
        ) || exit 1

        touch "$STAMP_DIR/libgcc-$PKGNAME"
    fi
}

GCC_CONFIGURE_OPTIONS=$BASE_HOST_OPTIONS
var_append GCC_CONFIGURE_OPTIONS "--target=$TARGET_TAG"
if [ "$TARGET_MULTILIBS" ]; then
    var_append GCC_CONFIGURE_OPTIONS "--enable-targets=all"
fi
var_append GCC_CONFIGURE_OPTIONS "--enable-languages=c,c++"
var_append GCC_CONFIGURE_OPTIONS "--with-sysroot=$INSTALL_DIR"

# A bug in MinGW-w64 forces us to build and use widl.
build_mingw_tools mingw-w64-tools
build_mingw_headers mingw-w64-headers

build_core_gcc gcc-$GCC_VERSION $GCC_CONFIGURE_OPTIONS

CRT_CONFIGURE_OPTIONS="--host=$TARGET_TAG --with-sysroot=$INSTALL_DIR --prefix=$PREFIX_FOR_TARGET"
if [ "$TARGET_MULTILIBS" ]; then
    var_append CRT_CONFIGURE_OPTIONS "--enable-lib32"
fi

build_mingw_crt mingw-w64-crt $CRT_CONFIGURE_OPTIONS

build_libgcc gcc-$GCC_VERSION

if [ "$PACKAGE_DIR" ]; then
    mkdir -p $PACKAGE_DIR
    fail_panic "Could not create packaging directory: $PACKAGE_DIR"
    PACKAGE_NAME=$PACKAGE_DIR/$TARGET_TAG-$OS-$HOST_ARCH.tar.bz2
    log "Packaging $TARGET_TAG toolchain to $PACKAGE_NAME"
    run tar cjf $PACKAGE_NAME -C $(dirname $INSTALL_DIR) $TARGET_TAG/
    fail_panic "Could not package $TARGET_TAG toolchain!"
    log "Done. See $PACKAGE_DIR:"
    ls -l $PACKAGE_NAME
else
    log "Done. See: $INSTALL_DIR"
fi

if [ "$CLEANUP" ]; then
    log "Cleaning up..."
    rm -rf $TEMP_DIR/*
fi

exit 0
