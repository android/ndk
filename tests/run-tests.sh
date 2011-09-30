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
ROOTDIR=`cd $PROGDIR/.. && pwd`
. $ROOTDIR/build/core/ndk-common.sh

# The list of tests that are too long to be part of a normal run of
# run-tests.sh. Most of these do not run properly at the moment.
LONG_TESTS="prebuild-stlport test-stlport test-gnustl"

#
# Parse options
#
VERBOSE=no
ABI=armeabi
PLATFORM=""
NDK_ROOT=
JOBS=$BUILD_NUM_CPUS
find_program ADB_CMD adb
TESTABLES="samples build device awk"
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
        --abi=*)
            ABI="$optarg"
            ;;
        --platform=*)
            PLATFORM="$optarg"
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
        --only-awk)
            TESTABLES=awk
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
    echo "    --abi=<name>      Only run tests for the specific ABI [$ABI]"
    echo "    --platform=<name> Force API level for testing; platform=<android-x>"
    echo "    --adb=<file>      Specify adb executable for device tests"
    echo "    --only-samples    Only rebuild samples"
    echo "    --only-build      Only rebuild build tests"
    echo "    --only-device     Only rebuild & run device tests"
    echo "    --only-awk        Only run awk tests."
    echo "    --full            Run all device tests, even very long ones."
    echo ""
    echo "NOTE: You cannot use --ndk and --package at the same time."
    echo ""
    echo "You can use --test=<name> several times to run several tests"
    echo "during the same invokation."
    exit 0
fi

# Run a command in ADB.
#
# This is needed because "adb shell" does not return the proper status
# of the launched command, so we need to
#
adb_cmd ()
{
    local RET ADB_CMD_LOG
    # mktemp under Mac OS X requires the -t option
    ADB_CMD_LOG=$(mktemp -t XXXXXXXX)
    if [ $VERBOSE = "yes" ] ; then
        echo "$ADB_CMD shell $@"
        $ADB_CMD shell $@ ";" echo \$? | tee $ADB_CMD_LOG
    else
        $ADB_CMD shell $@ ";" echo \$? > $ADB_CMD_LOG
    fi
    # Get last line in log, should be OK or KO
    # +Get rid of \r at the end of lines
    RET=`sed -e 's![[:cntrl:]]!!g' $ADB_CMD_LOG | tail -n1`
    rm -f $ADB_CMD_LOG
    return $RET
}

# Make a directory path on device
#
# The 'mkdir' command on the Android device does not
# support the '-p' option. This function will test
# for the existence of the parent directory and recursively
# call itself until it files a parent which exists; then
# create the requested directory.
adb_cmd_mkdir ()
{
    local FULLDIR BASEDIR
    FULLDIR=$1
    BASEDIR=`dirname $FULLDIR`

    #run $ADB_CMD shell "ls $BASEDIR 1>/dev/null 2>&1"
    adb_cmd "ls $BASEDIR 1>/dev/null 2>&1"
    if [ $? != 0 ] ; then
        if [ $BASEDIR = "/" ] ; then
            dump "ERROR: Could not find the root (/) directory on the device!"
            exit 1
        else
            adb_cmd_mkdir $BASEDIR
            adb_cmd_mkdir $FULLDIR
        fi
    else
        #run $ADB_CMD shell "mkdir $FULLDIR"
        # If the directory doesn't exist, make it
        adb_cmd "ls $FULLDIR 1>/dev/null 2>&1 || mkdir $FULLDIR"
        if [ $? != 0 ] ; then
            dump "ERROR: Could not mkdir '$FULLDIR' on the device!"
            exit 1
        fi
    fi
}

# Returns 0 if a variable containing one or more items separated
# by spaces contains a given value.
# $1: variable name (e.g. FOO)
# $2: value to test
var_list_contains ()
{
    echo `var_value $1` | tr ' ' '\n' | grep -q -F -x -e "$2"
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


TEST_DIR="/tmp/ndk-$USER/tests"
mkdir -p $TEST_DIR
setup_default_log_file "$TEST_DIR/build-tests.log"

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

BUILD_DIR=$TEST_DIR/build
mkdir -p "$BUILD_DIR" && rm -rf "$BUILD_DIR/*"

###
### RUN AWK TESTS
###

# Run a simple awk script
# $1: awk script to run
# $2: input file
# $3: expected output file
# $4+: optional additional command-line arguments for the awk command
run_awk_test ()
{
    local SCRIPT="$1"
    local SCRIPT_NAME="`basename $SCRIPT`"
    local INPUT="$2"
    local INPUT_NAME="`basename $INPUT`"
    local EXPECTED="$3"
    local EXPECTED_NAME="`basename $EXPECTED`"
    shift; shift; shift;
    local OUTPUT="$BUILD_DIR/$EXPECTED_NAME"
    if [ "$VERBOSE2" = "yes" ]; then
        echo "### COMMAND: awk -f \"$SCRIPT\" $@ < \"$INPUT\" > \"$OUTPUT\""
    fi
    awk -f "$SCRIPT" $@ < "$INPUT" > "$OUTPUT"
    fail_panic "Can't run awk script: $SCRIPT"
    if [ "$VERBOSE2" = "yes" ]; then
        echo "OUTPUT FROM SCRIPT:"
        cat "$OUTPUT"
        echo "EXPECTED VALUES:"
        cat "$EXPECTED"
    fi
    cmp -s "$OUTPUT" "$EXPECTED"
    if [ $? = 0 ] ; then
        echo "Awk script: $SCRIPT_NAME: passed $INPUT_NAME"
        if [ "$VERBOSE2" = "yes" ]; then
            cat "$OUTPUT"
        fi
    else
        if [ "$VERBOSE" = "yes" ]; then
            run diff -burN "$EXPECTED" "$OUTPUT"
        fi
        echo "Awk script: $SCRIPT_NAME: $INPUT_NAME FAILED!!"
        rm -f "$OUTPUT"
        exit 1
    fi
}

run_awk_test_dir ()
{
    local SCRIPT_NAME="`basename \"$DIR\"`"
    local SCRIPT="$ROOTDIR/build/awk/$SCRIPT_NAME.awk"
    local INPUT
    local OUTPUT
    if [ ! -f "$SCRIPT" ]; then
        echo "Awk script: $SCRIPT_NAME: Missing script: $SCRIPT"
        continue
    fi
    for INPUT in `ls "$PROGDIR"/awk/$SCRIPT_NAME/*.in`; do
        OUTPUT=`echo $INPUT | sed 's/\.in$/.out/g'`
        if [ ! -f "$OUTPUT" ]; then
            echo "Awk script: $SCRIPT_NAME: Missing awk output file: $OUTPUT"
            continue
        fi
        run_awk_test "$SCRIPT" "$INPUT" "$OUTPUT"
    done
}

if is_testable awk; then
    AWKDIR="$ROOTDIR/build/awk"
    for DIR in `ls -d "$PROGDIR"/awk/*`; do
        run_awk_test_dir "$DIR"
    done
fi

###
###  REBUILD ALL SAMPLES FIRST
###

# Special case, if ABI is 'armeabi' or 'armeabi-v7a'
# we want to build both armeabi and armeabi-v7a machine code
# even if we will only run the armeabi test programs on the
# device. This is done by not forcing the definition of APP_ABI
NDK_BUILD_FLAGS="-B"
case $ABI in
    armeabi|armeabi-v7a)
        ;;
    x86)
        NDK_BUILD_FLAGS="$NDK_BUILD_FLAGS APP_ABI=$ABI"
        ;;
    *)
        echo "ERROR: Unsupported abi value: $ABI"
        exit 1
        ;;
esac

# Force all tests to run at one API level
if [ "$PLATFORM" != "" ]; then
    NDK_BUILD_FLAGS="$NDK_BUILD_FLAGS APP_PLATFORM=$PLATFORM"
fi

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
    if [ -f "$DIR/BROKEN_BUILD" ] ; then
        echo "Skipping $1: (build)"
        return 0
    fi
    cp -r "$1" "$DIR"
    cd "$DIR" && run_ndk_build $NDK_BUILD_FLAGS
    if [ $? != 0 ] ; then
        echo "!!! BUILD FAILURE [$1]!!! See $NDK_LOGFILE for details or use --verbose option!"
        exit 1
    fi
}

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
        if [ -f $1/build.sh ]; then
            export NDK
            run $1/build.sh "$NDK_BUILD_FLAGS"
            if [ $? != 0 ]; then
                echo "!!! BUILD FAILURE [$1]!!! See $NDK_LOGFILE for details or use --verbose option!"
                exit 1
            fi
        else
            build_project $1
        fi
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
        # Do not build test if BROKEN_BUILD is defined
        if [ -f "$1/BROKEN_BUILD" ] ; then
            echo "Skipping broken device test build: `basename $1`"
            return 0
        fi
        echo "Building NDK device test: `basename $1`"
        build_project $1
    }

    run_device_test ()
    {
        local SRCDIR="$BUILD_DIR/`basename $1`/libs/$ABI"
        local DSTDIR="$2/ndk-tests"
        local SRCFILE
        local DSTFILE
        local PROGRAMS=
        local PROGRAM
        # Do not run the test if BROKEN_RUN is defined
        if [ -f "$1/BROKEN_RUN" -o -f "$1/BROKEN_BUILD" ] ; then
            dump "Skipping NDK device test run: `basename $1`"
            return 0
        fi
        if [ ! -d "$SRCDIR" ]; then
            dump "Skipping NDK device test run (no $ABI binaries): `basename $1`"
            return 0
        fi
        # First, copy all files to /data/local, except for gdbserver
        # or gdb.setup.
        adb_cmd_mkdir $DSTDIR
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
            adb_cmd LD_LIBRARY_PATH="$DSTDIR" $PROGRAM
            if [ $? != 0 ] ; then
                dump "   ---> TEST FAILED!!"
            fi
        done
        # Cleanup
        adb_cmd rm -r $DSTDIR
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
        ADB_DEVICES=`$ADB_CMD devices`
        log2 "ADB devices: $ADB_DEVICES"
        ADB_DEVCOUNT=`echo "$ADB_DEVICES" | wc -l`
        ADB_DEVCOUNT=`expr $ADB_DEVCOUNT - 1`
        log2 "ADB Device count: $ADB_DEVCOUNT"
        if [ "$ADB_DEVCOUNT" = "0" ]; then
            dump "WARNING: No device connected to adb!"
            SKIP_TESTS=yes
        elif [ "$ADB_DEVCOUNT" != 1 -a -z "$ANDROID_SERIAL" ] ; then
            dump "WARNING: More than one device connected to adb. Please define ANDROID_SERIAL!"
            SKIP_TESTS=yes
        fi
        echo "$ADB_DEVICES" | grep -q -e "offline"
        if [ $? = 0 ] ; then
            dump "WARNING: Device is offline, can't run device tests!"
            SKIP_TESTS=yes
        fi
    fi

    if [ "$SKIP_TESTS" = "yes" ] ; then
        dump "SKIPPING RUNNING TESTS ON DEVICE!"
    else
        for DIR in `ls -d $ROOTDIR/tests/device/*`; do
            log "Running device test: $DIR"
            if is_buildable $DIR; then
                run_device_test $DIR /data/local
            fi
        done
    fi
fi

dump "Cleaning up..."
rm -rf $BUILD_DIR
dump "Done."
