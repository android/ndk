#!/bin/sh
#
# Copyright (C) 2012 The Android Open Source Project
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
#  This shell script is used to copy clang tool "scan-build" and "scan-view"
#  for the Android NDK.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <ndk-dir> <toolchain>"

PROGRAM_DESCRIPTION=\
"Copy clang static code analyzer for the Android NDK.

Where <src-dir> is the location of toolchain sources, <ndk-dir> is
the top-level NDK installation path and <toolchain> is the name of
the toolchain to use (e.g. llvm-3.1)."

RELEASE=`date +%Y%m%d`

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Create archive tarball in specific directory"

extract_parameters "$@"

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

    SCAN_BUILD_SRC_DIR=$SRC_DIR/$TOOLCHAIN/clang/tools/scan-build
    if [ ! -d "$SCAN_BUILD_SRC_DIR" ] ; then
        echo "ERROR: Source directory does not contain scan-build: $SCAN_BUILD_SRC_DIR"
        exit 1
    fi

    SCAN_VIEW_SRC_DIR=$SRC_DIR/$TOOLCHAIN/clang/tools/scan-view
    if [ ! -d "$SCAN_VIEW_SRC_DIR" ] ; then
        echo "ERROR: Source directory does not contain scan-view: $SCAN_VIEW_SRC_DIR"
        exit 1
    fi

    LICENSE_FILE=$SRC_DIR/$TOOLCHAIN/clang/LICENSE.TXT
    if [ ! -f "$LICENSE_FILE" ] ; then
        echo "ERROR: Source directory does not contain clang license file: $LICENSE_FILE"
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
        fail_panic "Could not create target NDK installation path: $NDK_DIR"
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

# copy scan_build and scan_view
SCAN_BUILD_SUBDIR="prebuilt/common/scan-build"
run copy_directory "$SCAN_BUILD_SRC_DIR" "$NDK_DIR/$SCAN_BUILD_SUBDIR"
cp -p "$LICENSE_FILE" "$NDK_DIR/$SCAN_BUILD_SUBDIR"
rm -f $NDK_DIR/$SCAN_BUILD_SUBDIR/scan-build.1

SCAN_VIEW_SUBDIR="prebuilt/common/scan-view"
run copy_directory "$SCAN_VIEW_SRC_DIR" "$NDK_DIR/$SCAN_VIEW_SUBDIR"
cp -p "$LICENSE_FILE" "$NDK_DIR/$SCAN_VIEW_SUBDIR"

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE="scan-build-view.tar.bz2"
    dump "Packaging $ARCHIVE"
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SCAN_BUILD_SUBDIR" "$SCAN_VIEW_SUBDIR"
fi

dump "Done."
