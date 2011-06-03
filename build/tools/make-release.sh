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
# This script is used to build an NDK release package *from* scratch !!
# While handy, this is *not* the best way to generate NDK release packages.
# See docs/DEVELOPMENT.TXT for details.
#

. `dirname $0`/prebuilt-common.sh

force_32bit_binaries

# The default release name (use today's date)
RELEASE=`date +%Y%m%d`
register_var_option "--release=<name>" RELEASE "Specify release name"

# The package prefix
PREFIX=android-ndk
register_var_option "--prefix=<name>" PREFIX "Specify package prefix"

# Find the location of the platforms root directory
DEVELOPMENT_ROOT=`dirname $ANDROID_NDK_ROOT`/development/ndk
register_var_option "--development=<path>" DEVELOPMENT_ROOT "Path to development/ndk directory"

# Default location for final packages
OUT_DIR=/tmp/ndk-$USER/release
register_var_option "--out-dir=<path>" OUT_DIR "Path to output directory"

# Force the build
FORCE=no
register_var_option "--force" FORCE "Force build (do not ask initial question)"

# Use --incremental to implement incremental release builds.
# This is only useful to debug this script or the ones it calls.
INCREMENTAL=no
register_var_option "--incremental" INCREMENTAL "Enable incremental packaging (debug only)."

DARWIN_SSH=
if [ "$HOST_OS" = "linux" ] ; then
register_var_option "--darwin-ssh=<hostname>" DARWIN_SSH "Specify Darwin hostname to ssh to for the build."
fi

# Determine the host platforms we can build for.
# This is the current host platform, and eventually windows if
# we are on Linux and have the mingw32 compiler installed and
# in our path.
#
HOST_SYSTEMS="$HOST_TAG"

MINGW_GCC=
if [ "$HOST_TAG" == "linux-x86" ] ; then
    find_program MINGW_GCC i586-mingw32msvc-gcc
    if [ -n "$MINGW_GCC" ] ; then
        HOST_SYSTEMS="$HOST_SYSTEMS windows"
    fi
fi
if [ -n "$DARWIN_SSH" ] ; then
    HOST_SYSTEMS="$HOST_SYSTEMS darwin-x86"
fi

register_var_option "--systems=<list>" HOST_SYSTEMS "List of host systems to build for"

TOOLCHAIN_SRCDIR=
register_var_option "--toolchain-src-dir=<path>" TOOLCHAIN_SRCDIR "Use toolchain sources from <path>"

extract_parameters "$@"

# Print a warning and ask the user if he really wants to do that !
#
if [ "$FORCE" = "no" -a "$INCREMENTAL" = "no" ] ; then
    echo "IMPORTANT WARNING !!"
    echo ""
    echo "This script is used to generate an NDK release package from scratch"
    echo "for the following host platforms: $HOST_SYSTEMS"
    echo ""
    echo "This process is EXTREMELY LONG and may take SEVERAL HOURS on a dual-core"
    echo "machine. If you plan to do that often, please read docs/DEVELOPMENT.TXT"
    echo "that provides instructions on how to do that more easily."
    echo ""
    echo "Are you sure you want to do that [y/N] "
    read YESNO
    case "$YESNO" in
        y|Y|yes|YES)
            ;;
        *)
            echo "Aborting !"
            exit 0
    esac
fi

PROGRAM_PARAMETERS=
PROGRAM_DESCRIPTION=\
"This script is used to generate an NDK release package from scratch.

This process is EXTREMELY LONG and consists in the following steps:

  - downloading toolchain sources from the Internet
  - patching them appropriately (if needed)
  - rebuilding the toolchain binaries for the host system
  - rebuilding the platforms and samples directories from ../development/ndk
  - packaging everything into a host-specific archive

This can take several hours on a dual-core machine, even assuming a very
nice Internet connection and plenty of RAM and disk space.

Note that on Linux, if you have the 'mingw32' package installed, the script
will also automatically generate a windows release package. You can prevent
that by using the --platforms option.

IMPORTANT:
        If you intend to package NDK releases often, please read the
        file named docs/DEVELOPMENT.TXT which provides ways to do that
        more quickly, by preparing toolchain binary tarballs that can be
        reused for each package operation. This will save you hours of
        your time compared to using this script!
"

# Create directory where everything will be performed.
RELEASE_DIR=$NDK_TMPDIR/release-$RELEASE
if [ "$INCREMENTAL" = "no" ] ; then
    rm -rf $RELEASE_DIR && mkdir -p $RELEASE_DIR
else
    if [ ! -d "$RELEASE_DIR" ] ; then
        echo "ERROR: Can't make incremental, missing release dir: $RELEASE_DIR"
        exit 1
    fi
fi


#
# Timestamp management
TIMESTAMP_DIR="$RELEASE_DIR/timestamps"
mkdir -p "$TIMESTAMP_DIR"
if [ "$INCREMENTAL" = "no" ] ; then
    run rm -rf "$TIMESTAMP_DIR/*"
fi

timestamp_set ()
{
    touch "$TIMESTAMP_DIR/$1"
}

timestamp_clear ()
{
    rm -f "$TIMESTAMP_DIR/$1"
}

timestamp_check ()
{
    if [ -f "$TIMESTAMP_DIR/$1" ] ; then
        return 1
    else
        return 0
    fi
}

# Step 1, If needed, download toolchain sources into a temporary directory
if [ -n "$TOOLCHAIN_SRCDIR" ] ; then
    dump "Using toolchain source directory: $TOOLCHAIN_SRCDIR"
    timestamp_set   toolchain-download-sources
else
    if timestamp_check toolchain-download-sources; then
        dump "Downloading toolchain sources..."
        TOOLCHAIN_SRCDIR="$RELEASE_DIR/toolchain-src"
        log "Using toolchain source directory: $TOOLCHAIN_SRCDIR"
        $ANDROID_NDK_ROOT/build/tools/download-toolchain-sources.sh "$TOOLCHAIN_SRCDIR"
        if [ "$?" != 0 ] ; then
            dump "ERROR: Could not download toolchain sources"
            exit 1
        fi
        timestamp_set   toolchain-download-sources
        timestamp_clear build-prebuilts
        timestamp_clear build-host-prebuilts
        timestamp_clear build-darwin-prebuilts
        timestamp_clear build-mingw-prebuilts
    fi
fi

# Step 2, build the host toolchain binaries and package them
if timestamp_check build-prebuilts; then
    PREBUILT_DIR="$RELEASE_DIR/prebuilt"
    if timestamp_check build-host-prebuilts; then
        dump "Building host toolchain binaries..."
        $ANDROID_NDK_ROOT/build/tools/rebuild-all-prebuilt.sh --toolchain-src-dir="$TOOLCHAIN_SRCDIR" --package-dir="$PREBUILT_DIR" --build-dir="$RELEASE_DIR/build"
        fail_panic "Can't build $HOST_SYSTEM binaries."
        timestamp_set build-host-prebuilts
    fi
    if [ -n "$DARWIN_SSH" ] ; then
        if timestamp_check build-darwin-prebuilts; then
            dump "Building Darwin prebuilts through ssh to $DARWIN_SSH..."
            $ANDROID_NDK_ROOT/build/tools/rebuild-all-prebuilt.sh --toolchain-src-dir="$TOOLCHAIN_SRCDIR" --package-dir="$PREBUILT_DIR" --darwin-ssh="$DARWIN_SSH"
            fail_panic "Can't build Darwin binaries!"
            timestamp_set build-darwin-prebuilts
        fi
    fi
    if [ -n "$MINGW_GCC" ] ; then
        if timestamp_check build-mingw-prebuilts; then
            dump "Building windows toolchain binaries..."
            $ANDROID_NDK_ROOT/build/tools/rebuild-all-prebuilt.sh --toolchain-src-dir="$TOOLCHAIN_SRCDIR" --package-dir="$PREBUILT_DIR" --build-dir="$RELEASE_DIR/build-mingw" --mingw
            fail_panic "Can't build windows binaries."
            timestamp_set build-mingw-prebuilt
        fi
    fi
    timestamp_set build-prebuilts
    timestamp_clear make-packages
fi

# Step 3, package a release with everything
if timestamp_check make-packages; then
    dump "Generating NDK release packages"
    $ANDROID_NDK_ROOT/build/tools/package-release.sh --release=$RELEASE --prefix=$PREFIX --out-dir="$OUT_DIR" --prebuilt-dir="$PREBUILT_DIR" --systems="$HOST_SYSTEMS" --development-root="$DEVELOPMENT_ROOT"
    if [ $? != 0 ] ; then
        dump "ERROR: Can't generate proper release packages."
        exit 1
    fi
    timestamp_set make-packages
fi

dump "All clear. Good work! See $OUT_DIR"
