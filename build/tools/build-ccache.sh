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
#  This shell script is used to download the sources of the ccache
#  tool that can be used to speed-up rebuilds of NDK binaries.
#
#  We use a special patched version of ccache 2.4 that works
#  well on Win32 and handles the dependency generation compiler
#  flags (-MMD -MP -MF) properly.
#
#  Beta versions of ccache 3.0 are supposed to do that as well but
#  have not been checked yet.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<ndk-dir>"
PROGRAM_DESCRIPTION="Rebuild the prebuilt ccache binary for the Android NDK toolchain."

CCACHE_VERSION=ccache-2.4-android-20070905
CCACHE_PACKAGE=$CCACHE_VERSION.tar.gz
DOWNLOAD_ROOT=http://android.git.kernel.org/pub
CCACHE_URL=$DOWNLOAD_ROOT/$CCACHE_PACKAGE

OPTION_PACKAGE=no

BUILD_OUT=`random_temp_directory`
OPTION_BUILD_OUT=
OPTION_FROM=

register_option "--from=<url>" do_from "Specify source package" "$PACKAGE"
register_option "--build-out=<path>" do_build_out "Set temporary build directory" "/tmp/<random>"

do_from () { CCACHE_URL=$1; CCACHE_PACKAGE=`basename $1`; }
do_build_out () { OPTION_BUILD_OUT=$1; }

extract_parameters $@

set_parameters ()
{
    if [ -n "$2" ] ; then
        echo "ERROR: Too many parameters. See --help for usage."
        exit 1
    fi

    NDK_DIR=$1
    if [ -z "$NDK_DIR" ] ; then
        echo "ERROR: Missing required ndk directory. See --help for usage."
        exit 1
    fi

    mkdir -p $NDK_DIR
    if [ $? != 0 ] ; then
        echo "ERROR: Could not create NDK target directory: $NDK_DIR"
        exit 1
    fi
}

set_parameters $PARAMETERS

prepare_host_flags

fix_option BUILD_OUT "$OPTION_BUILD_OUT" "build directory"

# Check for md5sum
check_md5sum

prepare_download

run rm -rf $BUILD_OUT && run mkdir -p $BUILD_OUT
if [ $? != 0 ] ; then
    echo "ERROR: Could not create build directory: $BUILD_OUT"
    exit 1
fi

dump "Getting sources from $CCACHE_URL"

download_file $CCACHE_URL $BUILD_OUT/$CCACHE_PACKAGE
if [ $? != 0 ] ; then
    dump "Could not download $CCACHE_URL"
    dump "Aborting."
    exit 1
fi

cd $BUILD_OUT && tar xzf $BUILD_OUT/$CCACHE_PACKAGE
if [ $? != 0 ] ; then
    dump "Could not unpack $CCACHE_PACKAGE in $BUILD_OUT"
    exit 1
fi

echo "Building ccache from sources..."
cd $BUILD_OUT/$CCACHE_VERSION && run make clean && run make unpack && run make build
if [ $? != 0 ] ; then
    dump "Could not build ccache in $BUILD_OUT"
fi

PREBUILT_DIR=$NDK_DIR/build/prebuilt/$HOST_TAG/ccache
mkdir -p $PREBUILT_DIR && cp -p $BUILD_OUT/$CCACHE_VERSION/ccache $PREBUILT_DIR
if [ $? != 0 ] ; then
    dump "Could not copy ccache binary!"
    exit 1
fi

dump "Done"
