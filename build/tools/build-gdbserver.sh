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
is the name of the toolchain to use (e.g. arm-linux-androideabi-4.6).

The final binary is placed under:

    <ndk-dir>/toolchains <toolchain>/prebuilt/gdbserver

NOTE: The --platform option is ignored if --sysroot is used."

VERBOSE=no

OPTION_BUILD_OUT=
BUILD_OUT=/tmp/ndk-$USER/build/gdbserver
register_option "--build-out=<path>" do_build_out "Set temporary build directory"
do_build_out () { OPTION_BUILD_OUT="$1"; }

PLATFORM=$DEFAULT_PLATFORM
register_var_option "--platform=<name>"  PLATFORM "Target specific platform"

SYSROOT=
if [ -d $TOOLCHAIN_PATH/sysroot ] ; then
  SYSROOT=$TOOLCHAIN_PATH/sysroot
fi
register_var_option "--sysroot=<path>" SYSROOT "Specify sysroot directory directly"

NOTHREADS=no
register_var_option "--disable-threads" NOTHREADS "Disable threads support"

GDB_VERSION=$DEFAULT_GDB_VERSION
register_var_option "--gdb-version=<name>" GDB_VERSION "Use specific gdb version."

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Archive binary into specific directory"

register_jobs_option

extract_parameters "$@"

setup_default_log_file

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

    SRC_DIR2="$SRC_DIR/gdb/gdb-$GDB_VERSION/gdb/gdbserver"
    if [ -d "$SRC_DIR2" ] ; then
        SRC_DIR="$SRC_DIR2"
        log "Found gdbserver source directory: $SRC_DIR"
    fi

    if [ ! -f "$SRC_DIR/gdbreplay.c" ] ; then
        echo "ERROR: Source directory does not contain gdbserver sources: $SRC_DIR"
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

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
fi

prepare_target_build

parse_toolchain_name $TOOLCHAIN
check_toolchain_install $NDK_DIR $TOOLCHAIN

# Check build directory
#
fix_sysroot "$SYSROOT"
log "Using sysroot: $SYSROOT"

if [ -n "$OPTION_BUILD_OUT" ] ; then
    BUILD_OUT="$OPTION_BUILD_OUT"
fi
log "Using build directory: $BUILD_OUT"
run mkdir -p "$BUILD_OUT"

# Copy the sysroot to a temporary build directory
BUILD_SYSROOT="$BUILD_OUT/sysroot"
run mkdir -p "$BUILD_SYSROOT"
run cp -RHL "$SYSROOT"/* "$BUILD_SYSROOT"

# Remove libthread_db to ensure we use exactly the one we want.
rm -f $BUILD_SYSROOT/usr/lib/libthread_db*
rm -f $BUILD_SYSROOT/usr/include/thread_db.h

if [ "$NOTHREADS" != "yes" ] ; then
    # We're going to rebuild libthread_db.o from its source
    # that is under sources/android/libthread_db and place its header
    # and object file into the build sysroot.
    LIBTHREAD_DB_DIR=$ANDROID_NDK_ROOT/sources/android/libthread_db/gdb-$GDB_VERSION
    if [ ! -d "$LIBTHREAD_DB_DIR" ] ; then
        dump "ERROR: Missing directory: $LIBTHREAD_DB_DIR"
        exit 1
    fi
    # Small trick, to avoid calling ar, we store the single object file
    # with an .a suffix. The linker will handle that seamlessly.
    run cp $LIBTHREAD_DB_DIR/thread_db.h $BUILD_SYSROOT/usr/include/
    run $TOOLCHAIN_PREFIX-gcc --sysroot=$BUILD_SYSROOT -o $BUILD_SYSROOT/usr/lib/libthread_db.o -c $LIBTHREAD_DB_DIR/libthread_db.c
    run $TOOLCHAIN_PREFIX-ar -r $BUILD_SYSROOT/usr/lib/libthread_db.a $BUILD_SYSROOT/usr/lib/libthread_db.o
    if [ $? != 0 ] ; then
        dump "ERROR: Could not compile libthread_db.c!"
        exit 1
    fi
fi

log "Using build sysroot: $BUILD_SYSROOT"

# configure the gdbserver build now
dump "Configure: $TOOLCHAIN gdbserver-$GDB_VERSION build."

case "$GDB_VERSION" in
    6.6)
        CONFIGURE_FLAGS="--with-sysroot=$BUILD_SYSROOT"
        ;;
    7.3.x)
        # This flag is required to link libthread_db statically to our
        # gdbserver binary. Otherwise, the program will try to dlopen()
        # the threads binary, which is not possible since we build a
        # static executable.
        CONFIGURE_FLAGS="--with-libthread-db=$BUILD_SYSROOT/usr/lib/libthread_db.a"
        # Disable libinproctrace.so which needs crtbegin_so.o and crtbend_so.o instead of
        # CRTBEGIN/END above.  Clean it up and re-enable it in the future.
        CONFIGURE_FLAGS=$CONFIGURE_FLAGS" --disable-inprocess-agent"
        ;;
    7.6)
        CONFIGURE_FLAGS="--with-libthread-db=$BUILD_SYSROOT/usr/lib/libthread_db.a"
        CONFIGURE_FLAGS=$CONFIGURE_FLAGS" --disable-inprocess-agent"
        ;;
    *)
        CONFIGURE_FLAGS=""
esac

cd $BUILD_OUT &&
export CC="$TOOLCHAIN_PREFIX-gcc --sysroot=$BUILD_SYSROOT" &&
export CFLAGS="-O2 $GDBSERVER_CFLAGS"  &&
export LDFLAGS="-static -Wl,-z,nocopyreloc -Wl,--no-undefined" &&
run $SRC_DIR/configure \
--host=$GDBSERVER_HOST \
$CONFIGURE_FLAGS
if [ $? != 0 ] ; then
    dump "Could not configure gdbserver build. See $TMPLOG"
    exit 1
fi

# build gdbserver
dump "Building : $TOOLCHAIN gdbserver."
cd $BUILD_OUT &&
run make -j$NUM_JOBS
if [ $? != 0 ] ; then
    dump "Could not build $TOOLCHAIN gdbserver. Use --verbose to see why."
    exit 1
fi

# install gdbserver
#
# note that we install it in the toolchain bin directory
# not in $SYSROOT/usr/bin
#
if [ "$NOTHREADS" = "yes" ] ; then
    DSTFILE="gdbserver-nothreads"
else
    DSTFILE="gdbserver"
fi
dump "Install  : $TOOLCHAIN $DSTFILE."
DEST=$ANDROID_NDK_ROOT/prebuilt/android-$ARCH/gdbserver
mkdir -p $DEST &&
run $TOOLCHAIN_PREFIX-objcopy --strip-unneeded $BUILD_OUT/gdbserver $DEST/$DSTFILE
if [ $? != 0 ] ; then
    dump "Could not install $DSTFILE. See $TMPLOG"
    exit 1
fi

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE=$ARCH-gdbserver.tar.bz2
    dump "Packaging: $ARCHIVE"
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$ANDROID_NDK_ROOT" "prebuilt/android-$ARCH/gdbserver/$DSTFILE"
fi

log "Cleaning up."
if [ -z "$OPTION_BUILD_OUT" ] ; then
    run rm -rf $BUILD_OUT
fi

dump "Done."
