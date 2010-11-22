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
#  This shell script is used to run all NDK build tests in a row.
#  "Build tests" are tests that check the building features of the NDK
#  but do not run anything on target devices/emulators.
#

#  You need to define the NDK

PROGDIR=`dirname $0`
PROGDIR=`cd $PROGDIR && pwd`

# Assume that we are under tests/
# and that the samples will be under samples/ and platforms/android-N/samples/
#
ROOTDIR=`dirname $PROGDIR`
. $ROOTDIR/build/core/ndk-common.sh

#
# Parse options
#
VERBOSE=no
NDK_ROOT=
JOBS=$BUILD_NUM_CPUS
find_program ADB_CMD adb
TESTABLES="samples build device"

while [ -n "$1" ]; do
    opt="$1"
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case "$opt" in
        --help|-h|-\?)
            OPTION_HELP=yes
            ;;
        --verbose)
            if [ "$VERBOSE" = "yes" ] ; then
                VERBOSE2=yes
            else
                VERBOSE=yes
            fi
            ;;
        --ndk=*)
            NDK_ROOT="$optarg"
            ;;
        -j*)
            JOBS=`expr "$opt" : '-j\(.*\)'`
            shift
            ;;
        --jobs=*)
            JOBS="$optarg"
            ;;
        --adb=*)
            ADB_CMD="$optarg"
            ;;
        --only-samples)
            TESTABLES=samples
            ;;
        --only-build)
            TESTABLES=build
            ;;
        --only-device)
            TESTABLES=device
            ;;
        -*) # unknown options
            echo "ERROR: Unknown option '$opt', use --help for list of valid ones."
            exit 1
        ;;
        *)  # Simply record parameter
            if [ -z "$PARAMETERS" ] ; then
                PARAMETERS="$opt"
            else
                PARAMETERS="$PARAMETERS $opt"
            fi
            ;;
    esac
    shift
done

if [ "$OPTION_HELP" = "yes" ] ; then
    echo "Usage: $PROGNAME [options]"
    echo ""
    echo "Run all NDK automated tests at once."
    echo ""
    echo "Valid options:"
    echo ""
    echo "    --help|-h|-?      Print this help"
    echo "    --verbose         Enable verbose mode"
    echo "    --ndk=<path>      Path to NDK to test [$ROOTDIR]"
    echo "    -j<N> --jobs=<N>  Launch parallel builds [$JOBS]"
    echo "    --adb=<file>      Specify adb executable for device tests"
    echo "    --only-samples    Only rebuild samples"
    echo "    --only-build      Only rebuild build tests"
    echo "    --only-device     Only rebuild & run device tests"
    echo ""
    exit 0
fi

#
# List of stuff to actually tests
#
is_testable () {
    echo "$TESTABLES" | tr ' ' '\n' | grep -q -e "^$1$"
    if [ $? = 0 ]; then
        return 0
    else
        return 1
    fi
}
#
# Check the NDK install path.
#
if [ -n "$NDK_ROOT" ] ; then
    if [ ! -d "$NDK_ROOT" ] ; then
        dump "ERROR: Your --ndk option does not point to a directory: $NDK_ROOT"
        dump "Please use a valid path for this option."
        exit 1
    fi
    if [ ! -f "$NDK_ROOT/ndk-build" -o ! -f "$NDK_ROOT/build/core/ndk-common.sh" ] ; then
        dump "ERROR: Your --ndk option does not point to a valid NDK install: $NDK_ROOT"
        dump "Please use a valid NDK install path for this option."
        exit 3
    fi
    NDK="$NDK_ROOT"
else
    NDK="$ROOTDIR"
fi

#
# Create log file
#

mkdir -p /tmp/ndk-tests
setup_default_log_file /tmp/ndk-tests/build-tests.log

BUILD_DIR=`mktemp -d /tmp/ndk-tests/build-XXXXXX`

run_ndk_build ()
{
    run $NDK/ndk-build -j$JOBS "$@"
}

build_project ()
{
    local NAME=`basename $1`
    local DIR="$BUILD_DIR/$NAME"
    cp -r "$1" "$DIR"
    cd "$DIR" && run_ndk_build -B V=1
    if [ $? != 0 ] ; then
        echo "!!! BUILD FAILURE [$1]!!! See $NDK_LOGFILE for details or use --verbose option!"
        exit 1
    fi
}

###
###  REBUILD ALL SAMPLES FIRST
###

#
# Determine list of samples directories.
#
if is_testable samples; then
    if [ -f "$NDK/RELEASE.TXT" ] ; then
        # This is a release package, all samples should be under $NDK/samples
        SAMPLES_DIRS="$NDK/samples"
        if [ ! -d "$SAMPLES_DIRS" ] ; then
            dump "ERROR: Missing samples directory: $SAMPLES_DIRS"
            dump "Your NDK release installation is broken!"
            exit 1
        fi
        log "Using release NDK samples from: $SAMPLES_DIRS"
    else
        # This is a development work directory, we will take the samples
        # directly from development/ndk.
        DEVNDK_DIR=`dirname $NDK`/development/ndk
        if [ ! -d "$DEVNDK_DIR" ] ; then
            dump "ERROR: Could not find development NDK directory: $DEVNDK_DIR"
            dump "Please clone platform/development.git from android.git.kernel.org"
            exit 1
        fi
        SAMPLES_DIRS="$DEVNDK_DIR/samples"
        for DIR in `ls -d $DEVNDK_DIR/platforms/android-*/samples`; do
            SAMPLES_DIRS="$SAMPLES_DIRS $DIR"
        done
        dump "Using development NDK samples from $DEVNDK_DIR"
        if [ "$VERBOSE" = "yes" ] ; then
            echo "$SAMPLES_DIRS" | tr ' ' '\n'
        fi
    fi

    #
    # Copy the samples to a temporary build directory

    build_sample ()
    {
        echo "Building NDK sample: `basename $1`"
        build_project $1
    }

    for DIR in $SAMPLES_DIRS; do
        for SUBDIR in `ls -d $DIR/*`; do
            if [ -f "$SUBDIR/jni/Android.mk" ] ; then
                build_sample $SUBDIR
            fi
        done
    done
fi

###
###  BUILD PROJECTS UNDER tests/build/
###

if is_testable build; then
    build_build_test ()
    {
        echo "Building NDK build test: `basename $1`"
        build_project $1
    }

    for DIR in `ls -d $ROOTDIR/tests/build/*`; do
        if [ -f "$DIR/jni/Android.mk" ] ; then
            build_build_test $DIR
        fi
    done
fi

###
###  BUILD PROJECTS UNDER tests/device/
###  XXX: TODO: RUN THEM ON A DEVICE/EMULATOR WITH ADB
###

if is_testable device; then
    build_device_test ()
    {
        echo "Building NDK device test: `basename $1`"
        build_project $1
    }

    for DIR in `ls -d $ROOTDIR/tests/device/*`; do
        if [ -f "$DIR/jni/Android.mk" ] ; then
            build_device_test $DIR
        fi
    done
fi

dump "Cleaning up..."
rm -rf $BUILD_DIR
dump "Done."
