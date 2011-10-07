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

CC=gcc
CUSTOM_CC=
register_option "--cc=<path>" do_cc "Specify host compiler"
do_cc () { CC=$1; CUSTOM_CC=yes; }

register_jobs_option

BUILD_DIR=
register_var_option "--build-dir=<path>" BUILD_DIR "Specify build directory"

OUT=$ANDROID_NDK_ROOT/$(get_host_exec_name ndk-stack)
CUSTOM_OUT=
register_option "--out=<file>" do_out "Specify output executable path" "$OUT"
do_out () { CUSTOM_OUT=true; OUT=$1; }

GNUMAKE=
register_var_option "--make=<path>" GNUMAKE "Specify GNU Make program"

DEBUG=
register_var_option "--debug" DEBUG "Build debug version"

register_mingw_option

extract_parameters "$@"

# Choose a build directory if not specified by --build-dir
if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR=$NDK_TMPDIR/build-ndk-stack
    log "Auto-config: --build-dir=$BUILD_DIR"
fi

# Choose a host compiler if not specified by --cc
# Note that --mingw implies we use the mingw32 cross-compiler.
if [ -z "$CUSTOM_CC" ]; then
    if [ "$MINGW" = "yes" ]; then
        CC=i586-mingw32msvc-g++
    else
        CC=g++
    fi
    log "Auto-config: --cc=$CC"
fi

if [ -z "$CUSTOM_OUT" ]; then
    OUT=$ANDROID_NDK_ROOT/$(get_host_exec_name ndk-stack)
    log "Auto-config: --out=$OUT"
fi

# GNU Make
if [ -z "$GNUMAKE" ]; then
    GNUMAKE=make
    log "Auto-config: --make=$GNUMAKE"
fi

PROGNAME=ndk-stack
if [ "$MINGW" = "yes" ]; then
    PROGNAME=$PROGNAME.exe
fi

# Create output directory
mkdir -p $(dirname $OUT)
if [ $? != 0 ]; then
    echo "ERROR: Could not create output directory: $(dirname $OUT)"
    exit 1
fi

SRCDIR=$ANDROID_NDK_ROOT/sources/host-tools/ndk-stack

# Let's roll
run $GNUMAKE -C $SRCDIR -f $SRCDIR/GNUMakefile \
    -B -j$NUM_JOBS \
    PROGNAME="$OUT" \
    BUILD_DIR="$BUILD_DIR" \
    CC="$CC" \
    DEBUG=$DEBUG

if [ $? != 0 ]; then
    echo "ERROR: Could not build host program!"
    exit 1
fi

log "Done!"
exit 0
