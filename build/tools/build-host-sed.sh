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
# Build the host version of the sed executable and place it
# at the right location

PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"Rebuild the host sed tool used by the NDK."

register_try64_option
register_mingw_option
register_jobs_option

OUT=
CUSTOM_OUT=
register_option "--out=<file>" do_out "Specify output executable path" "$OUT"
do_out () { CUSTOM_OUT=true; OUT=$1; }

GNUMAKE=make
register_var_option "--make=<path>" GNUMAKE "Specify GNU Make program"

extract_parameters "$@"

if [ -z "$CUSTOM_OUT" ]; then
    OUT=$ANDROID_NDK_ROOT/$(get_prebuilt_host_exec sed)
    log "Auto-config: --out=$OUT"
fi

SED_VERSION=4.2.1
SED_SRCDIR=$ANDROID_NDK_ROOT/sources/host-tools/sed-$SED_VERSION
if [ ! -d "$SED_SRCDIR" ]; then
    echo "ERROR: Can't find sed-$SED_VERSION source tree: $SED_SRCDIR"
    exit 1
fi

log "Using sources from: $SED_SRCDIR"

prepare_host_build

BUILD_DIR=$NDK_TMPDIR

log "Configuring the build"
mkdir -p $BUILD_DIR && rm -rf $BUILD_DIR/*
cd $BUILD_DIR &&
CFLAGS=$HOST_CFLAGS" -O2 -s" &&
export CC CFLAGS &&
run $SED_SRCDIR/configure --disable-nls --disable-rpath --disable-i18n --disable-acl
fail_panic "Failed to configure the sed-$SED_VERSION build!"

log "Building sed"
run $GNUMAKE -j $NUM_JOBS
fail_panic "Failed to build the sed-$SED_VERSION executable!"

log "Copying executable to prebuilt location"
run mkdir -p $(dirname "$OUT") && cp sed/$(get_host_exec_name sed) $OUT
fail_panic "Could not copy executable to: $OUT"

log "Cleaning up"
rm -rf $BUILD_DIR

log "Done."

