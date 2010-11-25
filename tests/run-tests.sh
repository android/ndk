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

# The list of tests that are too long to be part of a normal run of
# run-tests.sh
LONG_TESTS="prebuild-stlport test-stlport"

#
# Parse options
#
VERBOSE=no
NDK_ROOT=
JOBS=$BUILD_NUM_CPUS
find_program ADB_CMD adb
TESTABLES="samples build device"
FULL_TESTS=no
RUN_TESTS=
NDK_PACKAGE=

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
        --full)
            FULL_TESTS=yes;
            ;;
        --test=*)
            RUN_TESTS="$RUN_TESTS $optarg"
            ;;
        --package=*)
            NDK_PACKAGE="$optarg"
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
    echo "    --test=<name>     Run selected test (all if not used)"
    echo "    --ndk=<path>      Path to NDK to test [$ROOTDIR]"
    echo "    --package=<path>  Path to NDK package to test"
    echo "    -j<N> --jobs=<N>  Launch parallel builds [$JOBS]"
    echo "    --adb=<file>      Specify adb executable for device tests"
    echo "    --only-samples    Only rebuild samples"
    echo "    --only-build      Only rebuild build tests"
    echo "    --only-device     Only rebuild & run device tests"
    echo "    --full            Run all device tests, even very long ones."
    echo ""
    echo "NOTE: You cannot use --ndk and --package at the same time."
    echo ""
    echo "You can use --test=<name> several times to run several tests"
    echo "during the same invokation."
    exit 0
fi

# Run a command in ADB and return 0 in case of success, or 1 otherwise.
# This is needed because "adb shell" does not return the proper status
# of the launched command.
#
# NOTE: You must call set_adb_cmd_log before that to set the location
#        of the temporary log file that will be used.
#
adb_cmd ()
{
    local RET
    if [ -z "$ADB_CMD_LOG" ] ; then
        dump "INTERNAL ERROR: ADB_CMD_LOG not set!"
        exit 1
    fi
    if [ $VERBOSE = "yes" ] ; then
        echo "$ADB_CMD shell $@"
        $ADB_CMD shell "$@ && echo 'OK' || echo 'KO'" | tee $ADB_CMD_LOG
    else
        $ADB_CMD shell "$@ && echo 'OK' || echo 'KO'" > $ADB_CMD_LOG
    fi
    # Get last line in log, should be OK or KO
    RET=`tail -n1 $ADB_CMD_LOG`
    # Get rid of \r at the end of lines
    RET=`echo "$RET" | sed -e 's![[:cntrl:]]!!g'`
    [ "$RET" = "OK" ]
}

set_adb_cmd_log ()
{
    ADB_CMD_LOG="$1"
}

# Returns 0 if a variable containing one or more items separated
# by spaces contains a given value.
# $1: variable name (e.g. FOO)
# $2: value to test
var_list_contains ()
{
    echo `var_value $1` | tr ' ' '\n' | fgrep -q -e "$2"
}

#
# List of stuff to actually tests
#
is_testable () {
    var_list_contains TESTABLES "$1"
}

# is_buildable returns 0 if a test should be built/run for this invocation
# $1: test path
if [ -n "$RUN_TESTS" ] ; then
    is_buildable () {
        test -f $1/jni/Android.mk &&
        var_list_contains RUN_TESTS "`basename $1`"
    }
elif [ "$FULL_TESTS" = "yes" ] ; then
    is_buildable () {
        test -f $1/jni/Android.mk
    }
else # !FULL_TESTS
    is_buildable () {
        test -f $1/jni/Android.mk || return 1
        ! var_list_contains LONG_TESTS "`basename $1`" || return 1
    }
fi # !FULL_TESTS


mkdir -p /tmp/ndk-tests
setup_default_log_file /tmp/ndk-tests/build-tests.log

if [ -n "$NDK_PACKAGE" ] ; then
    if [ -n "$NDK_ROOT" ] ; then
        dump "ERROR: You can't use --ndk and --package at the same time!"
        exit 1
    fi
    NDK_ROOT=/tmp/ndk-tests/install
    mkdir -p  "$NDK_ROOT" && rm -rf "$NDK_ROOT/*"
    dump "Unpacking NDK package to $NDK_ROOT"
    unpack_archive "$NDK_PACKAGE" "$NDK_ROOT"
    NDK_ROOT=`ls -d $NDK_ROOT/*`
fi

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

BUILD_DIR=`mktemp -d /tmp/ndk-tests/build-XXXXXX`
set_adb_cmd_log "$BUILD_DIR/adb-cmd.log"

NDK_BUILD_FLAGS="-B"
# Use --verbose twice to see build commands for the tests
if [ "$VERBOSE2" = "yes" ] ; then
    NDK_BUILD_FLAGS="$NDK_BUILD_FLAGS V=1"
fi

run_ndk_build ()
{
    run $NDK/ndk-build -j$JOBS "$@"
}

build_project ()
{
    local NAME=`basename $1`
    local DIR="$BUILD_DIR/$NAME"
    cp -r "$1" "$DIR"
    cd "$DIR" && run_ndk_build $NDK_BUILD_FLAGS
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
            if is_buildable $SUBDIR; then
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
        if is_buildable $DIR; then
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

    run_device_test ()
    {
        local SRCDIR="$BUILD_DIR/`basename $1`/libs/armeabi"
        local DSTDIR="$2/ndk-tests"
        local SRCFILE
        local DSTFILE
        local PROGRAMS=
        local PROGRAM
        # First, copy all files to /data/local, except for gdbserver
        # or gdb.setup.
        $ADB_CMD shell mkdir $DSTDIR
        for SRCFILE in `ls $SRCDIR`; do
            DSTFILE=`basename $SRCFILE`
            if [ "$DSTFILE" = "gdbserver" -o "$DSTFILE" = "gdb.setup" ] ; then
                continue
            fi
            DSTFILE="$DSTDIR/$DSTFILE"
            run $ADB_CMD push "$SRCDIR/$SRCFILE" "$DSTFILE" &&
            run $ADB_CMD shell chmod 0755 $DSTFILE
            if [ $? != 0 ] ; then
                dump "ERROR: Could not install $SRCFILE to device!"
                exit 1
            fi
            # If its name doesn't end with .so, add it to PROGRAMS
            echo "$DSTFILE" | grep -q -e '\.so$'
            if [ $? != 0 ] ; then
                PROGRAMS="$PROGRAMS $DSTFILE"
            fi
        done
        for PROGRAM in $PROGRAMS; do
            dump "Running device test: `basename $PROGRAM`"
            adb_cmd $PROGRAM
            if [ $? != 0 ] ; then
                dump "   ---> TEST FAILED!!"
            fi
        done
        # Cleanup
        $ADB_CMD shell rm -r $DSTDIR
        #for SRCFILE in `ls $SRCDIR`; do
        #    DSTFILE=`basename $SRCFILE`
        #    $ADB_CMD shell rm $DSTDIR/$DSTFILE
        #done
    }

    for DIR in `ls -d $ROOTDIR/tests/device/*`; do
        if is_buildable $DIR; then
            build_device_test $DIR
        fi
    done

    # Do we have adb and any device connected here?
    # If not, we can't run our tests.
    #
    SKIP_TESTS=no
    if [ -z "$ADB_CMD" ] ; then
        dump "WARNING: No 'adb' in your path!"
        SKIP_TESTS=yes
    else
        ADB_DEVCOUNT=`$ADB_CMD devices | wc -l`
        ADB_DEVCOUNT=`expr $ADB_DEVCOUNT - 2`
        if [ "$ADB_DEVCOUNT" = "0" ]; then
            dump "WARNING: No device connected to adb!"
            SKIP_TESTS=yes
        elif [ "$ADB_DEVCOUNT" != 1 -a -z "$ADB_SERIAL" ] ; then
            dump "WARNING: More than one device connected to adb. Please define ADB_SERIAL!"
            SKIP_TESTS=yes
        fi
    fi

    if [ "$SKIP_TESTS" = "yes" ] ; then
        dump "SKIPPING RUNNING TESTS ON DEVICE!"
    else
        for DIR in `ls -d $ROOTDIR/tests/device/*`; do
            if is_buildable $DIR; then
                run_device_test $DIR /data/local
            fi
        done
    fi
fi

dump "Cleaning up..."
rm -rf $BUILD_DIR
dump "Done."
