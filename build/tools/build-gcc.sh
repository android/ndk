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
#  This shell script is used to rebuild the gcc and toolchain binaries
#  for the Android NDK.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <ndk-dir> <toolchain>"

PROGRAM_DESCRIPTION=\
"Rebuild the gcc toolchain prebuilt binaries for the Android NDK.

Where <src-dir> is the location of toolchain sources, <ndk-dir> is
the top-level NDK installation path and <toolchain> is the name of
the toolchain to use (e.g. arm-linux-androideabi-4.4.3)."

RELEASE=`date +%Y%m%d`
BUILD_OUT=/tmp/ndk-$USER/build/toolchain
OPTION_BUILD_OUT=
register_var_option "--build-out=<path>" OPTION_BUILD_OUT "Set temporary build directory"

# Note: platform API level 9 or higher is needed for proper C++ support
PLATFORM=$DEFAULT_PLATFORM
register_var_option "--platform=<name>"  PLATFORM "Specify platform name"

OPTION_SYSROOT=
register_var_option "--sysroot=<path>"   OPTION_SYSROOT   "Specify sysroot directory directly"

GDB_VERSION=$DEFAULT_GDB_VERSION
register_var_option "--gdb-version=<version>"  GDB_VERSION "Specify gdb version"

BINUTILS_VERSION=$DEFAULT_BINUTILS_VERSION
EXPLICIT_BINUTILS_VERSION=
register_option "--binutils-version=<version>" do_binutils_version "Specify binutils version" "$BINUTILS_VERSION"
do_binutils_version () {
    BINUTILS_VERSION=$1
    EXPLICIT_BINUTILS_VERSION=true
}

GMP_VERSION=$DEFAULT_GMP_VERSION
register_var_option "--gmp-version=<version>" GMP_VERSION "Specify gmp version"

MPFR_VERSION=$DEFAULT_MPFR_VERSION
register_var_option "--mpfr-version=<version>" MPFR_VERSION "Specify mpfr version"

MPC_VERSION=$DEFAULT_MPC_VERSION
register_var_option "--mpc-version=<version>" MPC_VERSION "Specify mpc version"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Create archive tarball in specific directory"

register_jobs_option
register_mingw_option
register_try64_option

extract_parameters "$@"

prepare_mingw_toolchain /tmp/ndk-$USER/build

fix_option BUILD_OUT "$OPTION_BUILD_OUT" "build directory"
setup_default_log_file $BUILD_OUT/config.log

set_parameters ()
{
    SRC_DIR="$1"
    NDK_DIR="$2"
    TOOLCHAIN="$3"

    # Check source directory
    #
    if [ -z "$SRC_DIR" ] ; then
        echo "ERROR: Missing source directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$SRC_DIR/gcc" ] ; then
        echo "ERROR: Source directory does not contain gcc sources: $SRC_DIR"
        exit 1
    fi

    log "Using source directory: $SRC_DIR"

    # Check NDK installation directory
    #
    if [ -z "$NDK_DIR" ] ; then
        echo "ERROR: Missing NDK directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$NDK_DIR" ] ; then
        mkdir -p $NDK_DIR
        if [ $? != 0 ] ; then
            echo "ERROR: Could not create target NDK installation path: $NDK_DIR"
            exit 1
        fi
    fi

    log "Using NDK directory: $NDK_DIR"

    # Check toolchain name
    #
    if [ -z "$TOOLCHAIN" ] ; then
        echo "ERROR: Missing toolchain name parameter. See --help for details."
        exit 1
    fi
}

set_parameters $PARAMETERS

prepare_target_build

parse_toolchain_name $TOOLCHAIN

fix_sysroot "$OPTION_SYSROOT"

check_toolchain_src_dir "$SRC_DIR"

if [ ! -d $SRC_DIR/gdb/gdb-$GDB_VERSION ] ; then
    echo "ERROR: Missing gdb sources: $SRC_DIR/gdb/gdb-$GDB_VERSION"
    echo "       Use --gdb-version=<version> to specify alternative."
    exit 1
fi

if [ -z "$EXPLICIT_BINUTILS_VERSION" ]; then
    BINUTILS_VERSION=$(get_default_binutils_version_for_gcc $TOOLCHAIN)
    dump "Auto-config: --binutils-version=$BINUTILS_VERSION"
fi

if [ ! -d $SRC_DIR/binutils/binutils-$BINUTILS_VERSION ] ; then
    echo "ERROR: Missing binutils sources: $SRC_DIR/binutils/binutils-$BINUTILS_VERSION"
    echo "       Use --binutils-version=<version> to specify alternative."
    exit 1
fi

fix_option MPFR_VERSION "$OPTION_MPFR_VERSION" "mpfr version"
if [ ! -f $SRC_DIR/mpfr/mpfr-$MPFR_VERSION.tar.bz2 ] ; then
    echo "ERROR: Missing mpfr sources: $SRC_DIR/mpfr/mpfr-$MPFR_VERSION.tar.bz2"
    echo "       Use --mpfr-version=<version> to specify alternative."
    exit 1
fi

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
fi

set_toolchain_ndk $NDK_DIR $TOOLCHAIN

dump "Using C compiler: $CC"
dump "Using C++ compiler: $CXX"

rm -rf $BUILD_OUT
mkdir -p $BUILD_OUT

# Location where the toolchain license files are
TOOLCHAIN_LICENSES=$ANDROID_NDK_ROOT/build/tools/toolchain-licenses

# Without option "--sysroot" (and its variations), GCC will attempt to
# search path specified by "--with-sysroot" at build time for headers/libs.
# Path at --with-sysroot contains minimal headers and libs to boostrap
# toolchain build, and it's not needed afterward (NOTE: NDK provides
# sysroot at specified API level,and Android build explicit lists header/lib
# dependencies.
#
# It's better to point --with-sysroot to local directory otherwise the
# path may be found at compile-time and bad things can happen: eg.
#  1) The path exists and contain incorrect headers/libs
#  2) The path exists at remote server and blocks GCC for seconds
#  3) The path exists but not accessible, which crashes GCC!
#
# For canadian build --with-sysroot has to be sub-directory of --prefix.
# Put TOOLCHAIN_BUILD_PREFIX to BUILD_OUT which is in /tmp by default,
# and TOOLCHAIN_BUILD_SYSROOT underneath.

TOOLCHAIN_BUILD_PREFIX=$BUILD_OUT/prefix
TOOLCHAIN_BUILD_SYSROOT=$TOOLCHAIN_BUILD_PREFIX/sysroot
dump "Sysroot  : Copying: $SYSROOT --> $TOOLCHAIN_BUILD_SYSROOT"
mkdir -p $TOOLCHAIN_BUILD_SYSROOT && (cd $SYSROOT && tar ch *) | (cd $TOOLCHAIN_BUILD_SYSROOT && tar x)
if [ $? != 0 ] ; then
    echo "Error while copying sysroot files. See $TMPLOG"
    exit 1
fi

# configure the toolchain
#
dump "Configure: $TOOLCHAIN toolchain build"
# Old versions of the toolchain source packages placed the
# configure script at the top-level. Newer ones place it under
# the build directory though. Probe the file system to check
# this.
BUILD_SRCDIR=$SRC_DIR/build
if [ ! -d $BUILD_SRCDIR ] ; then
    BUILD_SRCDIR=$SRC_DIR
fi
OLD_ABI="${ABI}"
export CC CXX
export CFLAGS_FOR_TARGET="$ABI_CFLAGS_FOR_TARGET"
export CXXFLAGS_FOR_TARGET="$ABI_CXXFLAGS_FOR_TARGET"
# Needed to build a 32-bit gmp on 64-bit systems
export ABI=$HOST_GMP_ABI
# -Wno-error is needed because our gdb-6.6 sources use -Werror by default
# and fail to build with recent GCC versions.
export CFLAGS="-Wno-error"

# This extra flag is used to slightly speed up the build
EXTRA_CONFIG_FLAGS="--disable-bootstrap"

# This is to disable GCC 4.6 specific features that don't compile well
# the flags are ignored for older GCC versions.
EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --disable-libquadmath --disable-plugin"

# Enable Gold as default
case "$TOOLCHAIN" in
    # Note that only ARM and X86 are supported
    x86-4.6|arm-linux-androideabi-4.6)
        EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --enable-gold=default"
    ;;
esac

#export LDFLAGS="$HOST_LDFLAGS"
cd $BUILD_OUT && run \
$BUILD_SRCDIR/configure --target=$ABI_CONFIGURE_TARGET \
                        --enable-initfini-array \
                        --host=$ABI_CONFIGURE_HOST \
                        --build=$ABI_CONFIGURE_BUILD \
                        --disable-nls \
                        --prefix=$TOOLCHAIN_BUILD_PREFIX \
                        --with-sysroot=$TOOLCHAIN_BUILD_SYSROOT \
                        --with-binutils-version=$BINUTILS_VERSION \
                        --with-mpfr-version=$MPFR_VERSION \
                        --with-mpc-version=$MPC_VERSION \
                        --with-gmp-version=$GMP_VERSION \
                        --with-gcc-version=$GCC_VERSION \
                        --with-gdb-version=$GDB_VERSION \
                        --with-gxx-include-dir=$TOOLCHAIN_BUILD_PREFIX/include/c++/$GCC_VERSION \
                        $EXTRA_CONFIG_FLAGS \
                        $ABI_CONFIGURE_EXTRA_FLAGS
if [ $? != 0 ] ; then
    dump "Error while trying to configure toolchain build. See $TMPLOG"
    exit 1
fi
ABI="$OLD_ABI"
# build the toolchain
dump "Building : $TOOLCHAIN toolchain [this can take a long time]."
cd $BUILD_OUT
export CC CXX
export ABI=$HOST_GMP_ABI
JOBS=$NUM_JOBS

while [ -n "1" ]; do
    run make -j$JOBS
    if [ $? = 0 ] ; then
        break
    else
        if [ "$MINGW" = "yes" ] ; then
            # Unfortunately, there is a bug in the GCC build scripts that prevent
            # parallel mingw builds to work properly on some multi-core machines
            # (but not all, sounds like a race condition). Detect this and restart
            # in less parallelism, until -j1 also fail
            JOBS=$((JOBS/2))
            if [ $JOBS -lt 1 ] ; then
                echo "Error while building mingw toolchain. See $TMPLOG"
                exit 1
            fi
            dump "Parallel mingw build failed - continuing in less parallelism -j$JOBS"
        else
            echo "Error while building toolchain. See $TMPLOG"
            exit 1
        fi
    fi
done

ABI="$OLD_ABI"

# install the toolchain to its final location
dump "Install  : $TOOLCHAIN toolchain binaries."
cd $BUILD_OUT && run make install
if [ $? != 0 ] ; then
    echo "Error while installing toolchain. See $TMPLOG"
    exit 1
fi

# copy to toolchain path
run copy_directory "$TOOLCHAIN_BUILD_PREFIX" "$TOOLCHAIN_PATH"

# don't forget to copy the GPL and LGPL license files
run cp -f $TOOLCHAIN_LICENSES/COPYING $TOOLCHAIN_LICENSES/COPYING.LIB $TOOLCHAIN_PATH

# remove some unneeded files
run rm -f $TOOLCHAIN_PATH/bin/*-gccbug
run rm -rf $TOOLCHAIN_PATH/info
run rm -rf $TOOLCHAIN_PATH/man
run rm -rf $TOOLCHAIN_PATH/share
run rm -rf $TOOLCHAIN_PATH/lib/gcc/$ABI_CONFIGURE_TARGET/*/install-tools
run rm -rf $TOOLCHAIN_PATH/lib/gcc/$ABI_CONFIGURE_TARGET/*/plugin
run rm -rf $TOOLCHAIN_PATH/libexec/gcc/$ABI_CONFIGURE_TARGET/*/install-tools
run rm -rf $TOOLCHAIN_PATH/lib32/libiberty.a
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/libiberty.a
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/*/libiberty.a
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/*/*/libiberty.a

# Remove libstdc++ for now (will add it differently later)
# We had to build it to get libsupc++ which we keep.
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/libstdc++.*
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/*/libstdc++.*
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/include/c++

# strip binaries to reduce final package size
run strip $TOOLCHAIN_PATH/bin/*
run strip $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/bin/*
run strip $TOOLCHAIN_PATH/libexec/gcc/*/*/cc1$HOST_EXE
run strip $TOOLCHAIN_PATH/libexec/gcc/*/*/cc1plus$HOST_EXE
run strip $TOOLCHAIN_PATH/libexec/gcc/*/*/collect2$HOST_EXE

# copy SOURCES file if present
if [ -f "$SRC_DIR/SOURCES" ]; then
    cp "$SRC_DIR/SOURCES" "$TOOLCHAIN_PATH/SOURCES"
fi

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE="$TOOLCHAIN-$HOST_TAG.tar.bz2"
    SUBDIR=$(get_toolchain_install_subdir $TOOLCHAIN $HOST_TAG)
    dump "Packaging $ARCHIVE"
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SUBDIR"
fi

dump "Done."
if [ -z "$OPTION_BUILD_OUT" ] ; then
    rm -rf $BUILD_OUT
fi
