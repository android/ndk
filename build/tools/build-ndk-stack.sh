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
# This script is used to rebuild the host 'ndk-stack' tool from the
# sources under sources/host-tools/ndk-stack.
#
# Note: The tool is installed under prebuilt/$HOST_TAG/bin/ndk-stack
#       by default.
#
PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"This script is used to rebuild the host ndk-stack binary program."

register_jobs_option

BUILD_DIR=
register_var_option "--build-dir=<path>" BUILD_DIR "Specify build directory"

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Place binary in NDK installation path"

GNUMAKE=
register_var_option "--make=<path>" GNUMAKE "Specify GNU Make program"

DEBUG=
register_var_option "--debug" DEBUG "Build debug version"

PROGNAME=ndk-stack
register_var_option "--program-name=<name>" PROGNAME "Alternate NDK tool program name"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Archive binary into specific directory"

register_canadian_option
register_try64_option

extract_parameters "$@"

prepare_host_build

# Choose a build directory if not specified by --build-dir
if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR=$NDK_TMPDIR/build-$PROGNAME
    log "Auto-config: --build-dir=$BUILD_DIR"
fi
prepare_canadian_toolchain $BUILD_DIR

OUT=$NDK_DIR/$(get_host_exec_name $PROGNAME)

# GNU Make
if [ -z "$GNUMAKE" ]; then
    GNUMAKE=make
    log "Auto-config: --make=$GNUMAKE"
fi

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
fi

# Create output directory
mkdir -p $(dirname $OUT)
if [ $? != 0 ]; then
    echo "ERROR: Could not create output directory: $(dirname $OUT)"
    exit 1
fi

SRCDIR=$ANDROID_NDK_ROOT/sources/host-tools/$PROGNAME

# Let's roll
export CFLAGS=$HOST_CFLAGS" -O2 -s"
export LDFLAGS=$HOST_LDFLAGS
run $GNUMAKE -C $SRCDIR -f $SRCDIR/GNUmakefile \
    -B -j$NUM_JOBS \
    PROGNAME="$OUT" \
    BUILD_DIR="$BUILD_DIR" \
    CC="$CXX" CXX="$CXX" \
    STRIP="$STRIP" \
    DEBUG=$DEBUG

if [ $? != 0 ]; then
    echo "ERROR: Could not build host program!"
    exit 1
fi

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE=$PROGNAME-$HOST_TAG.tar.bz2
    SUBDIR=$(get_host_exec_name $PROGNAME $HOST_TAG)
    dump "Packaging: $ARCHIVE"
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SUBDIR"
    fail_panic "Could not create package: $PACKAGE_DIR/$ARCHIVE from $OUT"
fi

log "Cleaning up"
rm -rf $BUILD_DIR

log "Done!"
exit 0
