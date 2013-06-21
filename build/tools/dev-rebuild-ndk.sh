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
#
# This script is used internally to re-build the toolchains and NDK.
# This is an engineer's tool... definitely not a production tool. Don't expect it to be
# flawelss... at least at this revision.

# Revamp of the 'ndk-buil.sh' based on feedback and NDK build
# processes used by Google to generate the NDK>

# Comment out the remote Mac OS X builds as it is currently
# non-functional due to a change in AOSP
# 3592767ce5ca9431eea728370c99d97aadb0800e

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
PROGDIR=`dirname $0`
PROGDIR=`cd $PROGDIR && pwd`


# Name of this NDK release
OPTION_NDK_RELEASE=`date +%Y%m%d`
register_var_option "--release=<rel_name>" OPTION_NDK_RELEASE "Version of release"

# Should we only Build for Linux platform?
OPTION_QUICK_BUILD="no"
register_var_option "--quick" OPTION_QUICK_BUILD "Only build the Linux basics"

# List of toolchains to package
OPTION_TOOLCHAINS="$DEFAULT_ARCH_TOOLCHAIN_NAME_arm,$DEFAULT_ARCH_TOOLCHAIN_NAME_x86,$DEFAULT_ARCH_TOOLCHAIN_NAME_mips"
register_var_option "--toolchains=<toolchain[,toolchain]>" OPTION_TOOLCHAINS "Toolchain(s) to package"

OPTION_TRY_64=
register_try64_option

OPTION_ALSO_64=
register_option "--also-64" do_ALSO_64 "Also build 64-bit host toolchain"
do_ALSO_64 () { OPTION_ALSO_64=" --also-64"; }

OPTION_SEPARATE_64=
register_option "--separate-64" do_SEPARATE_64 "Separate 64-bit host toolchain to its own package"
do_SEPARATE_64 ()
{
    if [ "$TRY64" = "yes" ]; then
        echo "ERROR: You cannot use both --try-64 and --separate-64 at the same time."
        exit 1
    fi
    OPTION_SEPARATE_64=" --separate-64";
}
# # Name of the Mac OS Build host
# MAC_BUILD_HOST="macdroid"
# register_var_option "--mac-host=<name>" MAC_BUILD_HOST "Hostname of the Mac OS X system"

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"Generate the NDK toolchain package."

extract_parameters "$@"

if [ "$TRY64" = "yes" ]; then
    OPTION_TRY_64=" --try-64"
fi

TOP=$PWD
TODAY=`date '+%Y%m%d'`
PACKAGE_DIR=$TOP/ndk-release-$TODAY
HOST_OS=`uname -s | tr '[:upper:]' '[:lower:]'`
# Set the list of Build Targets based on this Host OS
case "$HOST_OS" in
linux )
    # Build for Local Linux and Cross-compile for Windows (MINGW)
    if [ "$OPTION_QUICK_BUILD" = "yes" ]; then
       BUILD_TARGET_PLATFORMS="linux-x86"
    else
       BUILD_TARGET_PLATFORMS="linux-x86 windows"
    fi
    ;;
darwin )
    # Build for Local Mac OS X
    BUILD_TARGET_PLATFORMS="darwin-x86"
    ;;
esac

#export ANDROID_NDK_ROOT=$TOP/ndk
#export PATH=$PATH:$ANDROID_NDK_ROOT/build/tools
export VERBOSE=--verbose

# If BUILD_NUM_CPUS is not already defined in your environment,
# define it as the double of HOST_NUM_CPUS. This is used to
# run make commands in parallel, as in 'make -j$BUILD_NUM_CPUS'
#
if [ -z "$BUILD_NUM_CPUS" ] ; then
    if [ -e /proc/cpuinfo ] ; then
        HOST_NUM_CPUS=`cat /proc/cpuinfo | grep -c processor`
        export BUILD_NUM_CPUS=$(($HOST_NUM_CPUS * 2 * 8 / 10))
    elif [ -e /usr/sbin/sysctl ] ; then
        HOST_NUM_CPUS=`/usr/sbin/sysctl -n hw.ncpu`
        export BUILD_NUM_CPUS=$(($HOST_NUM_CPUS * 2 * 8 / 10))
    else
        export BUILD_NUM_CPUS=1
        echo "WARNING: Could not find Host Number CPUs; defaulting to BUILD_NUM_CPUS=${BUILD_NUM_CPUS}"
    fi
fi

# CLEAN
rm -rf /tmp/ndk-$USER/{build,tmp}


#######################################
# Get the Toolchain sources
#######################################

# Create a sha1 for any additional patches
PATCHES_SHA1=""
if [ -d "$PROGDIR/toolchain-patches" ]
then
    PATCHES_SHA1=`( find $PROGDIR/toolchain-patches -type f -print ) | \
        sort -f | xargs cat | git hash-object --stdin | cut -c1-7`
    PATCHES_SHA1="+${PATCHES_SHA1}"
fi

echo
echo "Checking for Toolchain sources"
NDK_SRC_DIR=/tmp/ndk-$USER/src/android-ndk-src-${TOOLCHAIN_GIT_DATE}${PATCHES_SHA1}
if [ ! -d $NDK_SRC_DIR ]
then
    echo "  Downloading Toolchain sources"
    mkdir -p `dirname $NDK_SRC_DIR`
    logfile="$TOP/download-toolchain-sources.log"
    rotate_log $logfile
    $PROGDIR/download-toolchain-sources.sh $NDK_SRC_DIR \
        > $logfile 2>&1
    fail_panic "Could not download toolchain sources!"
else
    echo "  Found existing $NDK_SRC_DIR"
fi


ARCHS=$DEFAULT_ARCHS

# Build the platform
echo
echo "Build the ndk/platforms directory"
logfile="$TOP/build-platforms.log"
rotate_log $logfile
$PROGDIR/gen-platforms.sh \
    $VERBOSE \
    --arch=$(spaces_to_commas $ARCHS)  \
    --minimal \
    --fast-copy > $logfile 2>&1
fail_panic "build-platforms.sh failed. Logfile in $logfile"

logfile="$TOP/rebuild-all.log"
rotate_log $logfile

# Rebuild all prebuilts for archs and platforms
for TARGET_PLATFORM in ${BUILD_TARGET_PLATFORMS}
do
    # Set the Arch specific variables
    case "$TARGET_PLATFORM" in
    linux-x86 )
        TARGET_PLATFORM_OS="Linux"
        TARGET_PLATFORM_FLAGS="--systems=$TARGET_PLATFORM"
        ;;
    windows )
        TARGET_PLATFORM_OS="Windows"
        TARGET_PLATFORM_FLAGS="--systems=$TARGET_PLATFORM"
        # Skip this Target Platform in Quick Build Mode
        if [ "$OPTION_QUICK_BUILD" = "yes" ]; then break ; fi
        ;;
    darwin-x86 )
        TARGET_PLATFORM_OS="Mac OS X"
        TARGET_PLATFORM_FLAGS=""
#        TARGET_PLATFORM_FLAGS="--darwin-ssh=$MAC_BUILD_HOST"
#        # Skip this Target Platform in Quick Build Mode
#        if [ "$OPTION_QUICK_BUILD" = "yes" ]; then break ; fi
        ;;
    esac

    # Rebuilt all prebuilts for the arch type
    echo
    echo "Rebuilding the toolchain and prebuilt tarballs for $TARGET_PLATFORM_OS"
    cd $TOP
    $PROGDIR/rebuild-all-prebuilt.sh \
        --arch=$(spaces_to_commas $ARCHS)  \
        --package-dir=$PACKAGE_DIR \
        $MPFR_VERSION $GDB_VERSION $BINUTILS_VERSION \
        $TARGET_PLATFORM_FLAGS \
        $VERBOSE \
        $OPTION_TRY_64 \
        $OPTION_ALSO_64 \
        $NDK_SRC_DIR >> $logfile 2>&1
    fail_panic "rebuild-all-prebuilt.sh failed. Logfile in $logfile"
done # with TARGET_PLATFORM

ALL_SYSTEMS=`echo ${BUILD_TARGET_PLATFORMS}`

echo
echo "Building the NDK release"
cd $TOP
logfile="$TOP/package-release.log"
rotate_log $logfile
$PROGDIR/package-release.sh \
    --prebuilt-dir=$PACKAGE_DIR \
    --systems="$ALL_SYSTEMS" \
    --out-dir=$PACKAGE_DIR \
    --arch=$(spaces_to_commas $ARCHS)  \
    --prefix=android-ndk-${OPTION_NDK_RELEASE} \
    $OPTION_TRY_64 \
    $OPTION_SEPARATE_64 \
    --no-git \
    $VERBOSE > $logfile 2>&1
fail_panic "package-release.sh failed. Logfile in $logfile"

echo
echo "Packaging the NDK Toolchain sources"
NDK_TOOLCHAIN_PKG=${PACKAGE_DIR}/toolchain-src.tar.bz2
(cd $NDK_SRC_DIR && tar cjf $NDK_TOOLCHAIN_PKG .)
fail_panic "Could not package NDK Toolchain sources!"

exit 0

