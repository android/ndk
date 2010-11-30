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
# build-platforms.sh
#
# This tool is used when packaging a new release, or when developing
# the NDK itself. It will populate DST ($NDK/platforms by default)
# with the content of SRC ($NDK/../development/ndk/platforms/ by default).
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
# For backwards compatibility:
#  $SRC/android-N/arch-A/usr/include --> $DST/android-N/arch-A/usr/include
#  $SRC/android-N/arch-A/usr/lib     --> $DST/android-N/arch-A/usr/lib
#
# Repeat after that for N+1, N+2, etc..
#

. `dirname $0`/../core/ndk-common.sh

# Return the list of platform supported from $1/platforms
# as a single space-separated list of levels. (e.g. "3 4 5 8 9")
# $1: source directory
extract_platforms_from ()
{
    if [ -d "$1" ] ; then
        (cd "$1/platforms" && ls -d android-*) | sed -e "s!android-!!" | tr '\n' ' '
    else
        echo ""
    fi
}

# Remove trailing path of a path
# $1: path
remove_trailing_slash () {
    echo $1 | sed -e 's!/$!!g'
}

# Reverse a file path directory
# foo -> .
# foo/bar -> ..
# foo/bar/zoo -> ../..
reverse_path ()
{
    local path cur item
    path=`remove_trailing_slash $1`
    cur="."
    if [ "$path" != "." ] ; then
        for item in `echo "$path" | tr '/' ' '`; do
            cur="../$cur"
        done
    fi
    echo `echo $cur | sed -e 's!/.$!!g'`
}

test_reverse_path ()
{
    rr=`reverse_path $1`
    if [ "$rr" != "$2" ] ; then
        echo "ERROR: reverse_path '$1' -> '$rr' (expected '$2')"
    fi
}

test_reverse_path . .
test_reverse_path ./ .
test_reverse_path foo ..
test_reverse_path foo/ ..
test_reverse_path foo/bar ../..
test_reverse_path foo/bar/ ../..
test_reverse_path foo/bar/zoo ../../..
test_reverse_path foo/bar/zoo/ ../../..

SRCDIR="../development/ndk"
DSTDIR="$ANDROID_NDK_ROOT"

ABIS="arm"
PLATFORMS=`extract_platforms_from "$SRCDIR"`

OPTION_HELP=no
OPTION_PLATFORMS=
OPTION_SRCDIR=
OPTION_DSTDIR=
OPTION_NO_SAMPLES=no
OPTION_NO_SYMLINKS=no

VERBOSE=no
VERBOSE2=no

for opt do
  optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
  case "$opt" in
  --help|-h|-\?) OPTION_HELP=yes
  ;;
  --verbose)
    if [ "$VERBOSE" = "yes" ] ; then
        VERBOSE2=yes
    else
        VERBOSE=yes
    fi
    ;;
  --src-dir=*)
    OPTION_SRCDIR="$optarg"
    ;;
  --dst-dir=*)
    OPTION_DSTDIR="$optarg"
    ;;
  --platform=*)
    OPTION_PLATFORM=$optarg
    ;;
  --abi=*)
    OPTION_ABI=$optarg
    ;;
  --no-samples)
    OPTION_NO_SAMPLES=yes
    ;;
  --no-symlinks)
    OPTION_NO_SYMLINKS=yes
    ;;
  *)
    echo "unknown option '$opt', use --help"
    exit 1
  esac
done

if [ $OPTION_HELP = "yes" ] ; then
    echo "Collect files from an Android NDK development tree and assemble"
    echo "the platform files appropriately into a final release structure."
    echo ""
    echo "options:"
    echo ""
    echo "  --help             Print this message"
    echo "  --verbose          Enable verbose messages"
    echo "  --src-dir=<path>   Source directory for development platform files [$SRCDIR]"
    echo "  --dst-dir=<path>   Destination directory [$DSTDIR]"
    echo "  --platform=<list>  Space-separated list of API levels [$PLATFORMS]"
    echo "  --abi=<list>       Space-separated list of ABIs [$ABIS]"
    echo "  --no-samples       Ignore samples"
    echo "  --no-symlinks      Don't create symlinks, copy files instead"
    echo ""
    exit 0
fi

if [ -n "$OPTION_SRCDIR" ] ; then
    SRCDIR="$OPTION_SRCDIR";
    if [ ! -d "$SRCDIR" ] ; then
        echo "ERROR: Source directory $SRCDIR does not exist !"
        exit 1
    fi
    if [ ! -d "$SRCDIR/platforms/android-3" ] ; then
        echo "ERROR: Invalid source directory: $SRCDIR"
        echo "Please make sure it contains platforms/android-3 etc..."
        exit 1
    fi
else
    SRCDIR=`dirname $ANDROID_NDK_ROOT`/development/ndk
    log "Using source directory: $SRCDIR"
fi

if [ -n "$OPTION_PLATFORM" ] ; then
    PLATFORMS="$OPTION_PLATFORM"
else
    # Build the list from the content of SRCDIR
    PLATFORMS=`extract_platforms_from "$SRCDIR"`
    log "Using platforms: $PLATFORMS"
fi


if [ -n "$OPTION_DSTDIR" ] ; then
    DSTDIR="$OPTION_DSTDIR"
else
    log "Using destination directory: $DSTDIR"
fi

if [ -n "$OPTION_ABI" ] ; then
    ABIS="$OPTION_ABI"
fi

log "Checking source platforms."
for PLATFORM in $PLATFORMS; do
    DIR="$SRCDIR/platforms/android-$PLATFORM"
    if [ ! -d $DIR ] ; then
        echo "ERROR: Directory missing: $DIR"
        echo "Please check your --platform=<list> option and try again."
        exit 2
    else
        log "  $DIR"
    fi
done

log "Checking source platform ABIs."
BAD_ABIS=
for ABI in $ABIS; do
    eval CHECK_$ABI=no
done
for PLATFORM in $PLATFORMS; do
    for ABI in $ABIS; do
        DIR="$SRCDIR/platforms/android-$PLATFORM/arch-$ABI"
        if [ -d $DIR ] ; then
            log "  $DIR"
            eval CHECK_$ABI=yes
        fi
    done
done

BAD_ABIS=
for ABI in $ABIS; do
    CHECK=`var_value CHECK_$ABI`
    log "  $ABI check: $CHECK"
    if [ "$CHECK" = no ] ; then
        if [ -z "$BAD_ABIS" ] ; then
            BAD_ABIS=$ABI
        else
            BAD_ABIS="$BAD_ABIS $ABI"
        fi
    fi
done

if [ -n "$BAD_ABIS" ] ; then
    echo "ERROR: Source directory doesn't support these ABIs: $BAD_ABIS"
    exit 3
fi

# $1: source directory (relative to $SRCDIR)
# $2: destination directory (relative to $DSTDIR)
# $3: description of directory contents (e.g. "sysroot" or "samples")
copy_src_directory ()
{
    local SDIR="$SRCDIR/$1"
    local DDIR="$DSTDIR/$2"
    if [ -d "$SDIR" ] ; then
        log "Copying $3 from \$SRC/$1 to \$DST/$2."
        mkdir -p "$DDIR" && (cd "$SDIR" && tar chf - *) | (tar xf - -C "$DDIR")
        if [ $? != 0 ] ; then
            echo "ERROR: Could not copy $3 directory $SDIR into $DDIR !"
            exit 5
        fi
    fi
}

# Create a symlink-copy of directory $1 into $2
# This function is recursive.
#
# $1: source directory (relative to $SRCDIR)
# $2: destination directory (relative to $DSTDIR)
symlink_src_directory ()
{
    mkdir -p "$DSTDIR/$2"
    local files=`cd $DSTDIR/$1 && ls -1p | grep -v -e '.*/'`
    local subdirs=`cd $DSTDIR/$1 && ls -1p | grep -e '.*/' | sed -e 's!/$!!g'`
    local file subdir rev
    rev=`reverse_path $1`
    for file in $files; do
        ln -s $rev/$1/$file $DSTDIR/$2/$file
    done
    for subdir in $subdirs; do
        symlink_src_directory $1/$subdir $2/$subdir
    done
}

# Copy platform sysroot and samples into your destination
#

# $SRC/android-$PLATFORM/include --> $DST/platforms/android-$PLATFORM/arch-$ABI/usr/include
# $SRC/android-$PLATFORM/arch-$ABI/include --> $DST/platforms/android-$PLATFORM/arch-$ABI/usr/include
# for compatibility:
# $SRC/android-$PLATFORM/arch-$ABI/usr/include --> $DST/platforms/android-$PLATFORM/arch-$ABI/usr/include



# $SRC/android-$PLATFORM/arch-$ABI/usr --> $DST/platforms/android-$PLATFORM/arch-$ABI/usr
# $SRC/android-$PLATFORM/samples       --> $DST/samples
#
rm -rf $DSTDIR/platforms && mkdir -p $DSTDIR/platforms
PREV_PLATFORM_DST=
for PLATFORM in $PLATFORMS; do
    NEW_PLATFORM=platforms/android-$PLATFORM
    PLATFORM_SRC=$NEW_PLATFORM
    PLATFORM_DST=$NEW_PLATFORM
    dump "Copying android-$PLATFORM platform files"
    if [ -n "$PREV_PLATFORM_DST" ] ; then
        if [ "$OPTION_NO_SYMLINKS" = "yes" ] ; then
            log "Copying \$DST/$PREV_PLATFORM_DST to \$DST/$PLATFORM_DST"
            #cp -r $DSTDIR/$PREV_PLATFORM_DST $DSTDIR/$PLATFORM_DST
            copy_directory "$DSTDIR/$PREV_PLATFORM_DST" "$DSTDIR/$PLATFORM_DST"
        else
            log "Symlink-copying \$DST/$PREV_PLATFORM_DST to \$DST/$PLATFORM_DST"
            symlink_src_directory $PREV_PLATFORM_DST $PLATFORM_DST
        fi
        if [ $? != 0 ] ; then
            echo "ERROR: Could not copy previous sysroot to $DSTDIR/$NEW_PLATFORM"
            exit 4
        fi
    fi
    for ABI in $ABIS; do
        SYSROOT=arch-$ABI/usr
        log "Copy $ABI sysroot files from \$SRC/android-$PLATFORM over \$DST/android-$PLATFORM/arch-$ABI"
        copy_src_directory $PLATFORM_SRC/include           $PLATFORM_DST/$SYSROOT/include "sysroot headers"
        copy_src_directory $PLATFORM_SRC/arch-$ABI/include $PLATFORM_DST/$SYSROOT/include "sysroot headers"
        copy_src_directory $PLATFORM_SRC/arch-$ABI/lib     $PLATFORM_DST/$SYSROOT/lib "sysroot libs"
        copy_src_directory $PLATFORM_SRC/$SYSROOT          $PLATFORM_DST/$SYSROOT "sysroot"
    done
    PREV_PLATFORM_DST=$PLATFORM_DST
done

if [ "$OPTION_NO_SAMPLES" = no ] ; then
    # Copy platform samples and generic samples into your destination
    #
    # $SRC/samples/ --> $DST/samples/
    # $SRC/android-$PLATFORM/samples/ --> $DST/samples
    #
    dump "Copying generic samples"
    rm -rf $DSTDIR/samples && mkdir -p $DSTDIR/samples
    copy_src_directory  samples samples samples

    for PLATFORM in $PLATFORMS; do
        dump "Copy android-$PLATFORM samples"
        # $SRC/platform-$PLATFORM/samples --> $DST/samples
        copy_src_directory platforms/android-$PLATFORM/samples samples samples
    done

    # Cleanup generated files in samples
    rm -rf "$DSTDIR/samples/*/obj"
    rm -rf "$DSTDIR/samples/*/libs"
fi

log "Done !"
