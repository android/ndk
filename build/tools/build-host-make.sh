#!/bin/sh
#
# Copyright (C) 2011 The Android Open Source Project
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
# Build the host version of the make executable and place it
# at the right location

PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"Rebuild the host GNU Make tool used by the NDK."

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Install to specific NDK directory"

register_try64_option
register_canadian_option
register_jobs_option

OUT=
CUSTOM_OUT=
register_option "--out=<file>" do_out "Specify output executable path" "$OUT"
do_out () { CUSTOM_OUT=true; OUT=$1; }

GNUMAKE=make
register_var_option "--make=<path>" GNUMAKE "Specify GNU Make program for the build"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Archive binaries into package directory"

extract_parameters "$@"

if [ -z "$CUSTOM_OUT" ]; then
    SUBDIR=$(get_prebuilt_host_exec make)
    OUT=$NDK_DIR/$SUBDIR
    log "Auto-config: --out=$OUT"
fi

GNUMAKE_VERSION=3.81
GNUMAKE_SRCDIR=$ANDROID_NDK_ROOT/sources/host-tools/make-$GNUMAKE_VERSION
if [ ! -d "$GNUMAKE_SRCDIR" ]; then
    echo "ERROR: Can't find make-$GNUMAKE_VERSION source tree: $GNUMAKE_SRCDIR"
    exit 1
fi

log "Using sources from: $GNUMAKE_SRCDIR"

prepare_abi_configure_build
prepare_host_build

TMP_SRCDIR=$NDK_TMPDIR/src

# We need to copy the sources to a temporary directory because
# the build system will modify some documentation files in the
# source directory. Sigh...
log "Copying sources to temporary directory: $TMP_SRCDIR"
mkdir -p "$TMP_SRCDIR" && copy_directory "$GNUMAKE_SRCDIR" "$TMP_SRCDIR"
fail_panic "Could not copy GNU Make sources to: $TMP_SRCDIR"

BUILD_DIR=$NDK_TMPDIR/build

CONFIGURE_FLAGS="--disable-nls --disable-rpath"
if [ "$MINGW" = "yes" ]; then
    # Required for a proper mingw cross compile
    CONFIGURE_FLAGS=$CONFIGURE_FLAGS" --host=i586-pc-mingw32"
fi

if [ "$DARWIN" = "yes" ]; then
    # Required for a proper darwin cross compile
    CONFIGURE_FLAGS=$CONFIGURE_FLAGS" --host=$ABI_CONFIGURE_HOST"
fi

log "Configuring the build"
mkdir -p $BUILD_DIR && rm -rf $BUILD_DIR/*
prepare_canadian_toolchain $BUILD_DIR
cd $BUILD_DIR &&
CFLAGS=$HOST_CFLAGS" -O2 -s" &&
export CC CFLAGS &&
run $TMP_SRCDIR/configure $CONFIGURE_FLAGS --build=$ABI_CONFIGURE_BUILD
fail_panic "Failed to configure the make-$GNUMAKE_VERSION build!"

log "Building make"
run $GNUMAKE -j $NUM_JOBS
fail_panic "Failed to build the make-$GNUMAKE_VERSION executable!"

log "Copying executable to prebuilt location"
run mkdir -p $(dirname "$OUT") && cp $(get_host_exec_name make) $OUT
fail_panic "Could not copy executable to: $OUT"

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE=ndk-make-$HOST_TAG.tar.bz2
    dump "Packaging: $ARCHIVE"
    mkdir -p "$PACKAGE_DIR" &&
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SUBDIR"
    fail_panic "Could not package archive: $PACKAGE_DIR/$ARCHIVE"
fi

log "Cleaning up"
rm -rf $BUILD_DIR $TMP_SRCDIR

log "Done."
