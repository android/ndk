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
# Build the host version of the yasm executable and place it
# at the right location

PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <ndk-dir>"
PROGRAM_DESCRIPTION=\
"Rebuild yasm tool used by the NDK."

register_try64_option
register_canadian_option
register_jobs_option

BUILD_OUT=/tmp/ndk-$USER/build/yasm
OPTION_BUILD_OUT=
register_var_option "--build-out=<path>" OPTION_BUILD_OUT "Set temporary build directory"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Archive binaries into package directory"

extract_parameters "$@"

set_parameters ()
{
    SRC_DIR="$1"
    NDK_DIR="$2"

    # Check source directory
    #
    if [ -z "$SRC_DIR" ] ; then
        echo "ERROR: Missing source directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$SRC_DIR/yasm" ] ; then
        echo "ERROR: Source directory does not contain llvm sources: $SRC_DIR/yasm"
        exit 1
    fi

    SRC_DIR=`cd $SRC_DIR; pwd`
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
    NDK_DIR=`cd $NDK_DIR; pwd`
    log "Using NDK directory: $NDK_DIR"
}

set_parameters $PARAMETERS

prepare_abi_configure_build
prepare_host_build

fix_option BUILD_OUT "$OPTION_BUILD_OUT" "build directory"
setup_default_log_file $BUILD_OUT/config.log

rm -rf $BUILD_OUT
mkdir -p $BUILD_OUT

log "Copying yasm sources to $BUILD_OUT/src"
mkdir -p "$BUILD_OUT/src" && copy_directory "$SRC_DIR/yasm" "$BUILD_OUT/src"
fail_panic "Could not copy yasm sources to: $BUILD_OUT/src"

CONFIGURE_FLAGS="--disable-nls --disable-rpath --prefix=$BUILD_OUT/prefix"
if [ "$MINGW" = "yes" ]; then
    # Required for a proper mingw cross compile
    CONFIGURE_FLAGS=$CONFIGURE_FLAGS" --host=i586-pc-mingw32"
fi

if [ "$DARWIN" = "yes" ]; then
    # Required for a proper darwin cross compile
    CONFIGURE_FLAGS=$CONFIGURE_FLAGS" --host=$ABI_CONFIGURE_HOST"
fi

prepare_canadian_toolchain $BUILD_OUT

CFLAGS=$HOST_CFLAGS" -O2 -s"
export CC CFLAGS

log "Configuring the build"
cd $BUILD_OUT/src && run ./autogen.sh $CONFIGURE_FLAGS --build=$ABI_CONFIGURE_BUILD
fail_panic "Couldn't run autogen.sh in $BUILD_OUT/yasm!"

log "Building yasm"
# build yasm in -j1 to avoid a race condition not well understood at this moment
# which causes failure with error message reads:
#   perfect.c: Duplicates keys!
#   make: *** [x86insn_nasm.c] Error 1
#   make: *** Waiting for unfinished jobs....
run make -j1 # -j$NUM_JOBS
fail_panic "Failed to build the $BUILD_OUT/yasm!"

log "Installing yasm"
run make install
fail_panic "Failed to install $BUILD_OUT/yasm!"

run rm -rf $BUILD_OUT/prefix/share

log "Stripping yasm"
test -z "$STRIP" && STRIP=strip
find $BUILD_OUT/prefix/bin -maxdepth 1 -type f -exec $STRIP {} \;

log "Copying yasm"
#run copy_directory "$BUILD_OUT/prefix" "$(get_prebuilt_install_prefix)"
SUBDIR=$(get_prebuilt_host_exec yasm)
OUT=$NDK_DIR/$SUBDIR
run mkdir -p $(dirname "$OUT") && cp $BUILD_OUT/prefix/bin/$(get_host_exec_name yasm) $OUT
fail_panic "Could not copy yasm"

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE=ndk-yasm-$HOST_TAG.tar.bz2
    dump "Packaging: $ARCHIVE"
    mkdir -p "$PACKAGE_DIR" &&
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SUBDIR"
    fail_panic "Could not package archive: $PACKAGE_DIR/$ARCHIVE"
fi

log "Cleaning up"
if [ -z "$OPTION_BUILD_OUT" ] ; then
    rm -rf $BUILD_OUT
fi
