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
# dev-platform-expand.sh
#
# This tool is used to expand the content of development/ndk/platforms
# into a _single_ directory containing all headers / libraries corresponding
# to a given API level and architecture.
#
# The idea is that the content of $SRC/android-N/ only contains stuff
# that is relevant to API level N, and not contain anything that is already
# provided by API level N-1, N-2, etc..
#
# More precisely, for each architecture A:
#  $SRC/android-N/include        --> $DST/android-N/arch-A/usr/include
#  $SRC/android-N/arch-A/include --> $DST/android-N/arch-A/usr/include
#  $SRC/android-N/arch-A/lib     --> $DST/android-N/arch-A/usr/lib
#
# If we target level $API, we're going to copy the contents of android-3 to
# android-$API to the destination directory.
#

. `dirname $0`/prebuilt-common.sh

# Return the list of platform supported from $1/platforms
# as a single space-separated list of levels. (e.g. "3 4 5 8 9")
# $1: source directory
extract_platforms_from ()
{
    local RET
    if [ -d "$1" ] ; then
        RET=$((cd "$1/platforms" && ls -d android-*) | sed -e "s!android-!!" | sort -g)
    else
        RET=""
    fi
    RET=$(echo $RET)  # converts newlines to spaces
    echo $RET
}

# The default platform is the last entry in the API_LEVELS default variable
PLATFORM=android-$(echo $API_LEVELS | tr ' ' '\n' | tail -1)
register_var_option "--platform=<level>" PLATFORM "Target API level"

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
"Uncompress the platform files (headers/libraries) correspond to a given
platform into a single directory. The main idea is that the files are stored
in a platform-specific way under SRC=$$NDK/../development/ndk, i.e.:

  \$SRC/platforms/android-3/  -> all files corresponding to API level 3
  \$SRC/platforms/android-4/  -> only new/modified files corresponding to API level 4
  \$SRC/platforms/android-5/  -> only new/modified files corresponding to API level 5
  ...

As an example, expanding android-5 would mean:

  1 - copy all files from android-3 to \$DST directory

  2 - copy all files from android-4 to \$DST, eventually overwriting stuff
      from android-3 that was modified in API level 4

  3 - copy all files from android-5 to \$DST, eventually overwriting stuff
      from android-4 that was modified in API level 5

The script 'dev-platform-compress.sh' can be used to perform the opposite
operation, and knows which files are part of which API level.
"

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

# Check platform (normalize it, i.e. android-9 -> 9}
PLATFORM=${PLATFORM##android-}
if [ ! -d "$SRCDIR/android-$PLATFORM" ]; then
    echo "ERROR: Platform directory doesn't exist: $SRCDIR/android-$PLATFORM"
    exit 1
fi
log "Using platform: $PLATFORM"

if [ ! -d "$DSTDIR" ]; then
    mkdir -p "$DSTDIR"
    if [ $? != 0 ]; then
        echo "ERROR: Could not create destination directory: $DSTDIR"
        exit 1
    fi
fi
log "Using destination directory: $DSTDIR"

# Handle architecture list
#
# We support both --arch and --abi for backwards compatibility reasons
# --arch is the new hotness, --abi is deprecated.
#
if [ -n "$OPTION_ARCH" ]; then
    OPTION_ARCH=$(commas_to_spaces $OPTION_ARCH)
fi

if [ -n "$OPTION_ABI" ] ; then
    echo "WARNING: --abi=<names> is deprecated. Use --arch=<names> instead!"
    OPTION_ABI=$(commas_to_spaces $OPTION_ABI)
    if [ -n "$OPTION_ARCH" -a "$OPTION_ARCH" != "$OPTION_ABI" ]; then
        echo "ERROR: You can't use both --abi and --arch with different values!"
        exit 1
    fi
    OPTION_ARCH=$OPTION_ABI
fi

ARCHS=$(commas_to_spaces $ARCHS)
log "Using architectures: $(commas_to_spaces $ARCHS)"

# log "Checking source platform architectures."
# BAD_ARCHS=
# for ARCH in $ARCHS; do
#     eval CHECK_$ARCH=no
# done
# for ARCH in $ARCHS; do
#     DIR="$SRCDIR/android-$PLATFORM/arch-$ARCH"
#     if [ -d $DIR ] ; then
#         log "  $DIR"
#         eval CHECK_$ARCH=yes
#     fi
# done
# 
# BAD_ARCHS=
# for ARCH in $ARCHS; do
#     CHECK=`var_value CHECK_$ARCH`
#     log "  $ARCH check: $CHECK"
#     if [ "$CHECK" = no ] ; then
#         if [ -z "$BAD_ARCHS" ] ; then
#             BAD_ARCHS=$ARCH
#         else
#             BAD_ARCHS="$BAD_ARCHS $ARCH"
#         fi
#     fi
# done
# 
# if [ -n "$BAD_ARCHS" ] ; then
#     echo "ERROR: Source directory doesn't support these ARCHs: $BAD_ARCHS"
#     exit 3
# fi

copy_optional_directory ()
{
    if [ -d "$1" ]; then
        copy_directory "$1" "$2"
    fi
}

# Copy platform sysroot and samples into your destination
#

# $SRC/android-$PLATFORM/include --> $DST/platforms/android-$PLATFORM/arch-$ARCH/usr/include
# $SRC/android-$PLATFORM/arch-$ARCH/include --> $DST/platforms/android-$PLATFORM/arch-$ARCH/usr/include
# for compatibility:
# $SRC/android-$PLATFORM/arch-$ARCH/usr/include --> $DST/platforms/android-$PLATFORM/arch-$ARCH/usr/include



# $SRC/android-$PLATFORM/arch-$ARCH/usr --> $DST/platforms/android-$PLATFORM/arch-$ARCH/usr
# $SRC/android-$PLATFORM/samples       --> $DST/samples
#
for LEVEL in $API_LEVELS; do
    if [ "$LEVEL" -gt "$PLATFORM" ]; then
        break
    fi
    log "Copying android-$LEVEL platform files"
    for ARCH in $ARCHS; do
        SDIR="$SRCDIR/android-$LEVEL"
        DDIR="$DSTDIR/android-$PLATFORM"
        if [ -d "$SDIR" ]; then
            copy_directory "$SDIR/include" "$DDIR/include"
        fi
        ARCH_SDIR="$SDIR/arch-$ARCH"
        ARCH_DDIR="$DDIR/arch-$ARCH"
        if [ -d "$ARCH_SDIR" ]; then
            copy_optional_directory "$ARCH_SDIR/include" "$ARCH_DDIR/include"
            copy_optional_directory "$ARCH_SDIR/lib"     "$ARCH_DDIR/lib"
            rm -f "$ARCH_DDIR"/lib/*.so
            copy_optional_directory "$ARCH_SDIR/symbols" "$ARCH_DDIR/symbols"
            rm -f "$ARCH_DDIR"/symbols/*.so.txt
        fi
    done
done

log "Done !"
exit 0
