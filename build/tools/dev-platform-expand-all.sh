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
# dev-platform-expand-all.sh
#
# Call dev-platform-expand.sh for all API levels.
#

. `dirname $0`/prebuilt-common.sh
PROGDIR=$(dirname $0)

# We take by default stuff from $NDK/../development/ndk
SRCDIR="$(cd $ANDROID_NDK_ROOT/../development/ndk/platforms && pwd)"
register_var_option "--src-dir=<path>" SRCDIR "Source for compressed platforms"

# The default destination directory is a temporary one
DSTDIR=/tmp/ndk-$USER/platforms
register_var_option "--dst-dir=<path>" DSTDIR "Destination directory"

# Default architecture, note we can have several ones here
ARCHS="arm,x86,mips"
register_var_option "--arch=<name>" ARCHS "List of target architectures"

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Call dev-platform-expand.sh for all API levels."

extract_parameters "$@"

# Check source directory
if [ ! -d "$SRCDIR" ] ; then
    echo "ERROR: Source directory doesn't exist: $SRCDIR"
    exit 1
fi
if [ ! -d "$SRCDIR/android-3" ]; then
    echo "ERROR: Source directory doesn't seem to be valid: $SRCDIR"
    exit 1
fi
log "Using source directory: $SRCDIR"
log "Using destination directory: $DSTDIR"
log "Using architectures: $ARCHS"

for PLATFORM in $API_LEVELS; do
    dump "Expanding files for android-$PLATFORM"
    $PROGDIR/dev-platform-expand.sh --platform=$PLATFORM --src-dir=$SRCDIR --dst-dir=$DSTDIR --arch=$(spaces_to_commas $ARCHS)
    fail_panic "Could not expand android-$PLATFORM files!"
done

log "Done! See $DSTDIR"
exit 0
