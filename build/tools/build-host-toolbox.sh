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
#  This shell script is used to rebuild the toolbox programs which sources
#  are under $NDK/sources/host-tools/toolbox
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
. `dirname $0`/builder-funcs.sh

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Rebuild the prebuilt host toolbox binaries for the Android NDK.

These are simple command-line programs used by the NDK build script.

By default, this will try to place the binaries inside the current NDK
directory, unless you use the --ndk-dir=<path> option.
"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Put prebuilt tarballs into <path>."

NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "Specify NDK root path for the build."

BUILD_DIR=
OPTION_BUILD_DIR=
register_var_option "--build-dir=<path>" OPTION_BUILD_DIR "Specify temporary build dir."

NO_MAKEFILE=
register_var_option "--no-makefile" NO_MAKEFILE "Do not use makefile to speed-up build"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Archive binaries into package directory"

register_jobs_option
register_try64_option

extract_parameters "$@"

# Handle NDK_DIR
if [ -z "$NDK_DIR" ] ; then
    NDK_DIR=$ANDROID_NDK_ROOT
    log "Auto-config: --ndk-dir=$NDK_DIR"
else
    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: NDK directory does not exists: $NDK_DIR"
        exit 1
    fi
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    BUILD_DIR=$NDK_TMPDIR/build-toolbox
    log "Auto-config: --build-dir=$BUILD_DIR"
    rm -rf $BUILD_DIR/* && mkdir -p $BUILD_DIR
else
    BUILD_DIR=$OPTION_BUILD_DIR
fi
mkdir -p "$BUILD_DIR"
fail_panic "Could not create build directory: $BUILD_DIR"

if [ -z "$NO_MAKEFILE" ]; then
    MAKEFILE=$BUILD_DIR/Makefile
else
    MAKEFILE=
fi

TOOLBOX_SRCDIR=$ANDROID_NDK_ROOT/sources/host-tools/toolbox

BUILD_WINDOWS_SOURCES=yes

if [ "$BUILD_WINDOWS_SOURCES" ]; then
    ORIGINAL_HOST_TAG=$HOST_TAG
    MINGW=yes
    handle_canadian_build
    prepare_canadian_toolchain $BUILD_DIR

    SUBDIR=$(get_prebuilt_install_prefix $HOST_TAG)/bin
    DSTDIR=$NDK_DIR/$SUBDIR
    mkdir -p "$DSTDIR"
    fail_panic "Could not create destination directory: $DSTDIR"

    # Build echo.exe
    HOST_TAG=$ORIGINAL_HOST_TAG
    builder_begin_host "$BUILD_DIR" "$MAKEFILE"
    builder_set_srcdir "$TOOLBOX_SRCDIR"
    builder_set_dstdir "$DSTDIR"
    builder_sources echo_win.c
    builder_host_executable echo
    builder_end

    # Build cmp.exe
    HOST_TAG=$ORIGINAL_HOST_TAG
    builder_begin_host "$BUILD_DIR" "$MAKEFILE"
    builder_set_srcdir "$TOOLBOX_SRCDIR"
    builder_set_dstdir "$DSTDIR"
    builder_sources cmp_win.c
    builder_host_executable cmp
    builder_end

    if [ "$PACKAGE_DIR" ]; then
        ARCHIVE=toolbox-$HOST_TAG.tar.bz2
        dump "Packaging : $ARCHIVE"
        pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SUBDIR/echo.exe" "$SUBDIR/cmp.exe"
        fail_panic "Could not package toolbox binaires"
    fi
fi
