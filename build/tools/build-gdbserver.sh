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
#  This shell script is used to rebuild the gdbserver binary from
#  the Android NDK's prebuilt binaries.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <ndk-dir> <toolchain>"

PROGRAM_DESCRIPTION=\
"Rebuild the gdbserver prebuilt binary for the Android NDK toolchain.

Where <src-dir> is the location of the gdbserver sources,
<ndk-dir> is the top-level NDK installation path and <toolchain>
is the name of the toolchain to use (e.g. arm-eabi-4.4.0).

NOTE: The --platform option is ignored if --sysroot is used."

RELEASE=`date +%Y%m%d`
BUILD_OUT=`mktemp -d /tmp/ndk-toolchain-XXX`
PLATFORM=android-3
VERBOSE=no
JOBS=$HOST_NUM_CPUS

OPTION_HELP=no
OPTION_BUILD_OUT=
OPTION_SYSROOT=

register_option "--build-out=<path>" do_build_out "Set temporary build directory" "/tmp/random"
register_option "--platform=<name>"  do_platform  "Target specific platform" "$PLATFORM"
register_option "--sysroot=<path>"   do_sysroot   "Specify sysroot directory directly."
register_option "-j<number>"         do_jobs      "Use <number> build jobs in parallel" "$JOBS"

do_build_out () { OPTION_BUILD_OUT=$1; }
do_platform () { OPTION_PLATFORM=$1; }
do_sysroot () { OPTION_SYSROOT=$1; }
do_jobs () { JOBS=$1; }

extract_parameters $@

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

    if [ ! -f "$SRC_DIR/gdbreplay.c" ] ; then
        echo "ERROR: Source directory does not contain gdb sources: $SRC_DIR"
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
        echo "ERROR: NDK directory does not exist: $NDK_DIR"
        exit 1
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

prepare_host_flags

parse_toolchain_name
check_toolchain_install $NDK_DIR

# Check build directory
#
fix_option BUILD_OUT "$OPTION_BUILD_OUT" "build out directory"
fix_option PLATFORM "$OPTION_PLATFORM" "platform"
fix_sysroot "$OPTION_SYSROOT"

# configure the gdbserver build now
dump "Configure: $TOOLCHAIN gdbserver build."
run mkdir -p $BUILD_OUT
OLD_CC="$CC"
OLD_CFLAGS="$CFLAGS"
OLD_LDFLAGS="$LDFLAGS"

INCLUDE_DIRS=\
"-I$TOOLCHAIN_PATH/lib/gcc/$ABI_TOOLCHAIN_PREFIX/$GCC_VERSION/include \
-I$SYSROOT/usr/include"
CRTBEGIN="$SYSROOT/usr/lib/crtbegin_static.o"
CRTEND="$SYSROOT/usr/lib/crtend_android.o"
LIBRARY_LDFLAGS="$CRTBEGIN -L$SYSROOT/usr/lib -lc -lm -lgcc $CRTEND "

cd $BUILD_OUT &&
export CC="$TOOLCHAIN_PREFIX-gcc" &&
export CFLAGS="-O2 -nostdinc -nostdlib -D__ANDROID__ -DANDROID -DSTDC_HEADERS $INCLUDE_DIRS"  &&
export LDFLAGS="-static -Wl,-z,nocopyreloc -Wl,--no-undefined $LIBRARY_LDFLAGS" &&
run $SRC_DIR/configure \
--host=${ABI_CONFIGURE_HOST} \
--with-sysroot=$SYSROOT
if [ $? != 0 ] ; then
    dump "Could not configure gdbserver build. See $TMPLOG"
    exit 1
fi
CC="$OLD_CC"
CFLAGS="$OLD_CFLAGS"
LDFLAGS="$OLD_LDFLAGS"

# build gdbserver
dump "Building : $TOOLCHAIN gdbserver."
cd $BUILD_OUT &&
run make -j$JOBS
if [ $? != 0 ] ; then
    dump "Could not build $TOOLCHAIN gdbserver. Use --verbose to see why."
    exit 1
fi

# install gdbserver
#
# note that we install it in the toolchain bin directory
# not in $SYSROOT/usr/bin
#
dump "Install  : $TOOLCHAIN gdbserver."
DEST=$TOOLCHAIN_PATH/bin
mkdir -p $DEST &&
run $TOOLCHAIN_PREFIX-objcopy --strip-unneeded $BUILD_OUT/gdbserver $TOOLCHAIN_PATH/bin/gdbserver
if [ $? != 0 ] ; then
    dump "Could not install gdbserver. See $TMPLOG"
    exit 1
fi

dump "Done."
if [ -n "$OPTION_BUILD_OUT" ] ; then
    rm -rf $BUILD_OUT
fi
