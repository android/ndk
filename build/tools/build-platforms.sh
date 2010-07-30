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
. `dirname $0`/../core/ndk-common.sh

extract_platforms_from ()
{
    (cd $SRCDIR/platforms && ls -d android-*) | sed -e "s!android-!!" | tr '\n' ' '
}

SRCDIR="../development/ndk"
DSTDIR="$ANDROID_NDK_ROOT"

ABIS="arm"
PLATFORMS=`extract_platforms_from $SRCDIR`

OPTION_HELP=no
OPTION_PLATFORMS=
OPTION_SRCDIR=
OPTION_DSTDIR=

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
    echo "  --help             print this message"
    echo "  --verbose          enable verbose messages"
    echo "  --src-dir=<path>   source directory for development platform files [$SRCDIR]"
    echo "  --dst-dir=<path>   destination directory [$DSTDIR]"
    echo "  --platform=<list>  space-separated list of API levels [$PLATFORMS]"
    echo "  --abi=<list>       space-separated list of ABIs [$ABIS]"
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
        echo "Please make sure it contains android-3 / android-4 etc..."
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
    PLATFORMS=`(cd $SRCDIR/platforms && ls -d android-*) | sed -e "s!android-!!" | tr '\n' ' '`
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
copy_directory ()
{
    local SDIR="$SRCDIR/$1"
    local DDIR="$DSTDIR/$2"
    log "SDIR=$SDIR DDIR=$DDIR"
    if [ -d "$SDIR" ] ; then
        log "Copying $3 from \$SRC/$1 to \$DST/$2."
        mkdir -p "$DDIR" && cp -rf "$SDIR"/* "$DDIR"
        if [ $? != 0 ] ; then
            echo "ERROR: Could not copy $3 directory $SDIR into $DDIR !"
            exit 5
        fi
    fi
}


# Copy platform sysroot and samples into your destination
#
# $SRC/android-$PLATFORM/arch-$ABI/usr --> $DST/platforms/android-$PLATFORM/arch-$ABI/usr
# $SRC/android-$PLATFORM/samples       --> $DST/samples
#
PREV_PLATFORM_DST=
for PLATFORM in $PLATFORMS; do
    NEW_PLATFORM=platforms/android-$PLATFORM
    PLATFORM_SRC=$NEW_PLATFORM
    PLATFORM_DST=$NEW_PLATFORM
    if [ -n "$PREV_PLATFORM_DST" ] ; then
        log "Copying \$DST/$PREV_PLATFORM_DST to \$DST/$PLATFORM_DST"
        cp -r "$DSTDIR/$PREV_PLATFORM_DST" "$DSTDIR/$PLATFORM_DST"
        if [ $? != 0 ] ; then
            echo "ERROR: Could not copy previous sysroot to $DSTDIR/$NEW_PLATFORM"
            exit 4
        fi
    fi
    for ABI in $ABIS; do
        SYSROOT=arch-$ABI/usr
        copy_directory $PLATFORM_SRC/$SYSROOT $PLATFORM_DST/$SYSROOT sysroot
    done
    PREV_PLATFORM_DST=$PLATFORM_DST
done

# Copy platform samples and generic samples into your destination
#
# $SRC/samples/ --> $DST/samples/
# $SRC/android-$PLATFORM/arch-$ABI/samples/ --> $DST/samples/
#
copy_directory  samples samples samples

for PLATFORM in $PLATFORMS; do
    # $SRC/platform-$PLATFORM/samples --> $DST/samples
    copy_directory platforms/android-$PLATFORM/samples samples samples
done

log "Done !"
