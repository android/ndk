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

# Rebuild all prebuilts. This requires that you have a toolchain source tree
#

. `dirname $0`/prebuilt-common.sh
PROGDIR=`dirname $0`

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Put binaries into NDK install directory"

BUILD_DIR=/tmp/ndk-$USER/build
register_var_option "--build-dir=<path>" BUILD_DIR "Specify temporary build directory"

ARCHS=$DEFAULT_ARCHS
register_var_option "--arch=<arch>" ARCHS "Specify target architectures"

SYSTEMS=$HOST_TAG32
if [ "$HOST_TAG32" = "linux-x86" ]; then
    SYSTEMS=$SYSTEMS",windows"
    # If darwin toolchain exist, build darwin too
    if [ -f "${DARWIN_TOOLCHAIN}-gcc" ]; then
        SYSTEMS=$SYSTEMS",darwin-x86"
    fi
fi
CUSTOM_SYSTEMS=
register_option "--systems=<list>" do_SYSTEMS "Specify host systems"
do_SYSTEMS () { CUSTOM_SYSTEMS=true; SYSTEMS=$1; }

ALSO_64=
register_option "--also-64" do_ALSO_64 "Also build 64-bit host toolchain"
do_ALSO_64 () { ALSO_64=yes; }

RELEASE=`date +%Y%m%d`
PACKAGE_DIR=/tmp/ndk-$USER/prebuilt-$RELEASE
register_var_option "--package-dir=<path>" PACKAGE_DIR "Put prebuilt tarballs into <path>."

DARWIN_SSH=
if [ "$HOST_OS" = "linux" ] ; then
register_var_option "--darwin-ssh=<hostname>" DARWIN_SSH "Specify Darwin hostname for remote build."
fi

register_try64_option

PROGRAM_PARAMETERS="<toolchain-src-dir>"
PROGRAM_DESCRIPTION=\
"This script is used to rebuild all host and target prebuilts from scratch.
You will need to give the path of a toolchain source directory, one which
is typically created with the download-toolchain-sources.sh script.

Unless you use the --ndk-dir option, all binaries will be installed to the
current NDK directory.

All prebuilts will then be archived into tarball that will be stored into a
specific 'package directory'. Unless you use the --package-dir option, this
will be: $PACKAGE_DIR

Please read docs/DEV-SCRIPTS-USAGE.TXT for more usage information about this
script.
"

extract_parameters "$@"

SRC_DIR="$PARAMETERS"
check_toolchain_src_dir "$SRC_DIR"

if [ "$DARWIN_SSH" -a -z "$CUSTOM_SYSTEMS" ]; then
    SYSTEMS=$SYSTEMS",darwin-x86"
fi

FLAGS=
if [ "$VERBOSE" = "yes" ]; then
    FLAGS=$FLAGS" --verbose"
fi
if [ "$VERBOSE2" = "yes" ]; then
    FLAGS=$FLAGS" --verbose"
fi
FLAGS=$FLAGS" --ndk-dir=$NDK_DIR"
FLAGS=$FLAGS" --package-dir=$PACKAGE_DIR"
FLAGS=$FLAGS" --arch=$(spaces_to_commas $ARCHS)"

HOST_FLAGS=$FLAGS" --systems=$(spaces_to_commas $SYSTEMS)"
if [ "$TRY64" = "yes" ]; then
    HOST_FLAGS=$HOST_FLAGS" --try-64"
fi
if [ "$DARWIN_SSH" ]; then
    HOST_FLAGS=$HOST_FLAGS" --darwin-ssh=$DARWIN_SSH"
fi

if [ "$ALSO_64" = "yes" -a "$TRY64" != "yes" ] ; then
    $PROGDIR/build-host-prebuilts.sh $HOST_FLAGS "$SRC_DIR" --try-64
    fail_panic "Could not build host prebuilts in 64-bit!"
fi
$PROGDIR/build-host-prebuilts.sh $HOST_FLAGS "$SRC_DIR"
fail_panic "Could not build host prebuilts!"

TARGET_FLAGS=$FLAGS

$PROGDIR/build-target-prebuilts.sh $TARGET_FLAGS "$SRC_DIR"
fail_panic "Could not build target prebuilts!"

echo "Done, see $PACKAGE_DIR:"
ls -l $PACKAGE_DIR

exit 0
