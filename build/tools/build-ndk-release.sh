#!/bin/sh
#
# Copyright (C) 2009 The Android Open Source Project
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
#
# This script is used internally to re-build the toolchains and NDK.
# This is an engineer's tool... definitely not a production tool. Don't expect it to be
# flawelss... at least at this revision.

# Revamp of the 'ndk-buil.sh' based on feedback and NDK build
# processes used by Google to generate the NDK>


# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
PROGDIR=`dirname $0`
PROGDIR=`cd $PROGDIR && pwd`


# Name of this NDK release
OPTION_NDK_RELEASE="r5x-eng"
register_var_option "--release=<rel_name>" OPTION_NDK_RELEASE "Version of release"

# Should we only Build for Linux platform?
OPTION_QUICK_BUILD="no"
register_var_option "--quick" OPTION_QUICK_BUILD "Only build the Linux basics"

# Name of the Mac OS Build host
MAC_BUILD_HOST="macdroid"
register_var_option "--mac-host=<name>" MAC_BUILD_HOST "Hostname of the Mac OS X system"

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"Generate the NDK toolchain package."

extract_parameters "$@"

TOP=$PWD
TODAY=`date '+%Y%m%d'`
PACKAGE_DIR=$TOP/ndk-release-$TODAY
#export ANDROID_NDK_ROOT=$TOP/ndk
#export PATH=$PATH:$ANDROID_NDK_ROOT/build/tools
export VERBOSE=--verbose

# If BUILD_NUM_CPUS is not already defined in your environment,
# define it as the double of HOST_NUM_CPUS. This is used to
# run make commands in parallel, as in 'make -j$BUILD_NUM_CPUS'
#
if [ -z "$BUILD_NUM_CPUS" ] ; then
    HOST_NUM_CPUS=`cat /proc/cpuinfo | grep -c processor`
    export BUILD_NUM_CPUS=`expr $HOST_NUM_CPUS`
fi

# CLEAN
rm -rf /tmp/ndk-release
rm -rf /tmp/ndk-release-*
rm -rf /tmp/ndk-toolchain/*
rm -rf /tmp/android-toolchain*
rm -rf /tmp/android-ndk-*


#######################################
# Get the Toolchain sources
#######################################

# Create a sha1 for any additional patches
PATCHES_SHA1=""
if [ -d "$PROGDIR/toolchain-patches" ]
then
    PATCHES_SHA1=`( cd $PROGDIR/toolchain-patches ; find . -print ) | sort -f | git hash-object --stdin | cut -c1-7`
    PATCHES_SHA1="+${PATCHES_SHA1}"
fi

echo
echo "Checking for Toolchain sources"
NDK_SRC_DIR=/tmp/ndk/src/android-ndk-src-${TOOLCHAIN_GIT_DATE}${PATCHES_SHA1}
if [ ! -d $NDK_SRC_DIR ]
then
    echo "  Downloading Toolchain sources"
    mkdir -p $NDK_SRC_DIR
    $PROGDIR/download-toolchain-sources.sh $NDK_SRC_DIR \
        > $TOP/download-toolchain-sources.log 2>&1
    fail_panic "Could not download toolchain sources!"
else
    echo "  Found existing $NDK_SRC_DIR"
fi


ALL_ARCH=""
ALL_SYSTEMS=""
for ARCH in arm x86
do
    # Collect all Arch types for packaging
    ALL_ARCH="${ALL_ARCH},$ARCH"

    # Set the Arch specific variables
    case "$ARCH" in
    arm )
        PRODUCT=generic

        unset MPFR_VERSION
        unset GDB_VERSION
        unset BINUTILS_VERSION
        ;;
    x86 )
        PRODUCT=generic_$ARCH

        MPFR_VERSION="--mpfr-version=2.4.1"
        GDB_VERSION="--gdb-version=6.6"
        BINUTILS_VERSION="--binutils-version=2.20.1"
        ;;
    esac


    # Ensure we have a Product output for the NDK build
    export ANDROID_PRODUCT_OUT=$TOP/out/target/product/$PRODUCT
    if [ ! -d $ANDROID_PRODUCT_OUT ]; then
        echo >&2 Rebuild for $PRODUCT first... or change PRODUCT in $0.
        exit 1
    fi

    # Build the platform
    echo
    echo "$ARCH: Build the ndk/platforms directory"
    $PROGDIR/build-platforms.sh \
        $VERBOSE \
        --arch=$ARCH --no-symlinks --no-samples > $TOP/$ARCH.build-platforms.log 2>&1
    fail_panic "build-platforms.sh failed. Logfile in $TOP/$ARCH.build-platforms.log"


    logfile="$TOP/$ARCH.rebuild-all.log"
    if [ -f $logfile ]; then rm -f $logfile; fi

    # Rebuild all prebuilts for the arch type and platforms
    for TARGET_PLATFORM in linux-x86 windows darwin-x86
    do
        # Set the Arch specific variables
        case "$TARGET_PLATFORM" in
        linux-x86 )
            TARGET_PLATFORM_OS="Linux"
            TARGET_PLATFORM_FLAGS=""
            ;;
        windows )
            TARGET_PLATFORM_OS="Windows"
            TARGET_PLATFORM_FLAGS="--mingw"
            # Skip this Target Platform in Quick Build Mode
            if [ "$OPTION_QUICK_BUILD" = "yes" ]; then break ; fi
            ;;
        darwin-x86 )
            TARGET_PLATFORM_OS="Mac OS X"
            TARGET_PLATFORM_FLAGS="--darwin-ssh=$MAC_BUILD_HOST"
            # Skip this Target Platform in Quick Build Mode
            if [ "$OPTION_QUICK_BUILD" = "yes" ]; then break ; fi
            ;;
        esac

        # Collect all Host System Platform types for packaging
        ALL_SYSTEMS="${ALL_SYSTEMS},$TARGET_PLATFORM"

        # Rebuilt all prebuilts for the arch type
        echo
        echo "$ARCH: Rebuilding the toolchain and prebuilt tarballs for $TARGET_PLATFORM_OS"
        cd $TOP
        $PROGDIR/rebuild-all-prebuilt.sh \
            --arch=$ARCH \
            --toolchain-src-dir=$NDK_SRC_DIR \
            --package-dir=$PACKAGE_DIR \
            $MPFR_VERSION $GDB_VERSION $BINUTILS_VERSION \
            $TARGET_PLATFORM_FLAGS \
            $VERBOSE >> $logfile 2>&1
        fail_panic "rebuild-all-prebuilt.sh failed. Logfile in $logfile"
    done # with TARGET_PLATFORM
done # with ARCH

# Remove the leading commas from the All Lists
ALL_ARCH=`echo $ALL_ARCH | cut -c2-`
ALL_SYSTEMS=`echo $ALL_SYSTEMS | cut -c2-`

echo
echo "Building the NDK release"
cd $TOP
$PROGDIR/package-release.sh \
    --prebuilt-dir=$PACKAGE_DIR \
    --systems=$ALL_SYSTEMS \
    --out-dir=$PACKAGE_DIR \
    --arch=$ALL_ARCH \
    --prefix=android-ndk-${OPTION_NDK_RELEASE} \
    --no-git \
    $VERBOSE > $TOP/package-release.log 2>&1
fail_panic "package-release.sh failed. Logfile in $TOP/package-release.log"

exit 0

