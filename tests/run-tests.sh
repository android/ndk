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
NDK_BUILDTOOLS_PATH=$ROOTDIR/build/tools
. $NDK_BUILDTOOLS_PATH/ndk-common.sh
. $NDK_BUILDTOOLS_PATH/prebuilt-common.sh

# Defining _NDK_TESTING_ALL_=yes to put armeabi-v7a-hard in its own libs/armeabi-v7a-hard
# directoy and tested separately from armeabi-v7a.  Some tests are now compiled with both
# APP_ABI=armeabi-v7a and APP_ABI=armeabi-v7a-hard. Without _NDK_TESTING_ALL_=yes, tests
# may fail to install due to race condition on the same libs/armeabi-v7a
if [ -z "$_NDK_TESTING_ALL_" ]; then
   _NDK_TESTING_ALL_=all
fi
export _NDK_TESTING_ALL_

# The list of tests that are too long to be part of a normal run of
# run-tests.sh. Most of these do not run properly at the moment.
LONG_TESTS="prebuild-stlport test-stlport test-gnustl-full \
test-stlport_shared-exception test-stlport_static-exception \
test-gnustl_shared-exception-full test-gnustl_static-exception-full \
test-googletest-full test-libc++-shared-full test-libc++-static-full"

#
# Parse options
#
VERBOSE=no
ABI=default
PLATFORM=""
NDK_ROOT=
JOBS=$BUILD_NUM_CPUS
find_program ADB_CMD adb
TESTABLES="samples build device awk"
FULL_TESTS=no
RUN_TESTS=
RUN_TESTS_FILTERED=
NDK_PACKAGE=
WINE=
CONTINUE_ON_BUILD_FAIL=
if [ -z "$TEST_DIR" ]; then
    TEST_DIR="/tmp/ndk-$USER/tests"
fi
if [ -z "$TARGET_TEST_SUBDIR" ]; then
    TARGET_TEST_SUBDIR="ndk-tests"
fi

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
        --test-dir=*)
            TEST_DIR="$optarg"
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
        --test-filtered=*)
            # same as --test but apply BROKEN_RUN too. Useful for projects with tons of test some of them can't run
            RUN_TESTS="$RUN_TESTS $optarg"
            RUN_TESTS_FILTERED="yes"
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
        --wine)
            WINE=yes
            ;;
        --continue-on-build-fail)
            CONTINUE_ON_BUILD_FAIL=yes
            ;;
        -*) # unknown options
            echo "ERROR: Unknown option '$opt', use --help for list of valid ones."
            exit 1
        ;;
        *)  # Simply record new test name
            RUN_TESTS=$RUN_TESTS" $opt"
            ;;
    esac
    shift
done

if [ "$OPTION_HELP" = "yes" ] ; then
    echo "Usage: $PROGNAME [options] [testname1 [testname2...]]"
    echo ""
    echo "Run NDK automated tests. Without any parameter, this will try to"
    echo "run all standard tests, except those are tagged broken. You can"
    echo "also select/enforce specific tests by listing their name on the"
    echo "command-line."
    echo ""
    echo "Valid options:"
    echo ""
    echo "    --help|-h|-?      Print this help"
    echo "    --verbose         Enable verbose mode (can be used several times)"
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
    echo "    --wine            Build all tests with wine on Linux"
    echo ""
    echo "NOTE: You cannot use --ndk and --package at the same time."
    echo ""
    exit 0
fi

# Run a command in ADB.
#
# This is needed because "adb shell" does not return the proper status
# of the launched command, so we need to add it to the output, and grab
# it after that.
# $1: Device name
# $2: Variable name that will contain the result
# $3+: Command options
adb_var_shell_cmd ()
{
    # We need a temporary file to store the output of our command
    local ADB_SHELL_CMD_LOG RET OUT
    local DEVICE=$1
    local VARNAME=$2
    shift; shift;
    ADB_SHELL_CMD_LOG=$(mktemp -t XXXXXXXX)
    # Run the command, while storing the standard output to ADB_SHELL_CMD_LOG
    # and appending the exit code as the last line.
    if [ $VERBOSE = "yes" ] ; then
        echo "$ADB_CMD -s \"$DEVICE\" shell \"$@\""
        $ADB_CMD -s "$DEVICE" shell "$@" ";" echo \$? | sed -e 's![[:cntrl:]]!!g' | tee $ADB_SHELL_CMD_LOG
    else
        $ADB_CMD -s "$DEVICE" shell "$@" ";" echo \$? | sed -e 's![[:cntrl:]]!!g' > $ADB_SHELL_CMD_LOG
    fi
    # Get last line in log, which contains the exit code from the command
    RET=`sed -e '$!d' $ADB_SHELL_CMD_LOG`
    # Get output, which corresponds to everything except the last line
    OUT=`sed -e '$d' $ADB_SHELL_CMD_LOG`
    rm -f $ADB_SHELL_CMD_LOG
    if [ "$VARNAME" != "" ]; then
        eval $VARNAME=\"\$OUT\"
    fi
    return $RET
}

# Make a directory path on device
#
# The 'mkdir' command on the Android device does not
# support the '-p' option. This function will test
# for the existence of the parent directory and recursively
# call itself until it files a parent which exists; then
# create the requested directory.
adb_shell_mkdir ()
{
    local FULLDIR BASEDIR
    local DEVICE=$1
    local FULLDIR=$2
    local BASEDIR=`dirname $FULLDIR`

    adb_var_shell_cmd "$DEVICE" "" "ls $BASEDIR 1>/dev/null 2>&1"
    if [ $? != 0 ] ; then
        if [ $BASEDIR = "/" ] ; then
            dump "ERROR: Could not find the root (/) directory on the device!"
            exit 1
        else
            adb_shell_mkdir "$DEVICE" $BASEDIR
            adb_shell_mkdir "$DEVICE" $FULLDIR
        fi
    else
        #If the directory doesn't exist, make it
        adb_var_shell_cmd "$DEVICE" "" "ls $FULLDIR 1>/dev/null 2>&1 || mkdir $FULLDIR"
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
        [ -f $1/build.sh -o -f $1/jni/Android.mk ] &&
        var_list_contains RUN_TESTS "`basename $1`"
    }
elif [ "$FULL_TESTS" = "yes" ] ; then
    is_buildable () {
        [ -f $1/build.sh -o -f $1/jni/Android.mk ]
    }
else # !FULL_TESTS
    is_buildable () {
        [ -f $1/build.sh -o -f $1/jni/Android.mk ] || return 1
        ! var_list_contains LONG_TESTS "`basename $1`" || return 1
    }
fi # !FULL_TESTS


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
    if [ ! -f "$NDK_ROOT/ndk-build" -o ! -f "$NDK_ROOT/build/tools/prebuilt-common.sh" ] ; then
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

#
# Add -link-native-binary to allow linking native binaries
#
if [ "$NDK_ABI_FILTER" != "${NDK_ABI_FILTER%%bc*}" ] ; then
  APP_LDFLAGS="$APP_LDFLAGS -Wl,-link-native-binary"
fi


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

NDK_BUILD_FLAGS="-B"
if [ "$WINE" ]; then
    case "$NDK_HOST_32BIT" in
        1|true)
            WINE=wine12
            ;;
        *)
            WINE=wine17
            NDK_BUILD_FLAGS=""  # make.exe -B hangs in wine > 1.2.x
            if [ "$NDK_TOOLCHAIN_VERSION" != "4.4.3" ] ; then
                APP_LDFLAGS="$APP_LDFLAGS -fuse-ld=mcld" # 64-bit ld.gold can't run in any wine!
            fi
            ;;
    esac
    find_program WINE_PROG $WINE
    fail_panic "Can't locate $WINE"
fi

# $1: output bitcode path
gen_empty_bitcode() {
    TEMP_FILE=`mktemp`
    mv $TEMP_FILE ${TEMP_FILE}.c
    run $NDK/$(get_llvm_toolchain_binprefix $DEFAULT_LLVM_VERSION)/clang -shared -target le32-none-ndk -emit-llvm -o $1 ${TEMP_FILE}.c
    rm -f ${TEMP_FILE}.c
}

# $1: output archive path
gen_empty_archive() {
    run ar crs $1
}

case $ABI in
    default)  # Let the APP_ABI in jni/Application.mk decide what to build
        ;;
    armeabi|armeabi-v7a|arm64-v8a|x86|x86_64|mips|mips64|armeabi-v7a-hard)
        NDK_BUILD_FLAGS="$NDK_BUILD_FLAGS APP_ABI=$ABI"
        ;;
    *)
        if [ -n "$(filter_out "$PREBUILT_ABIS" "$ABI")" ] && [ -n "$(find_ndk_unknown_archs)" ]; then
            ABI=$(find_ndk_unknown_archs)
            NDK_BUILD_FLAGS="$NDK_BUILD_FLAGS APP_ABI=$ABI"

            # Create those temporarily files to make testing happy
            GCC_TOOLCHAIN_VERSION=`cat $NDK/toolchains/llvm-$DEFAULT_LLVM_VERSION/setup.mk | grep '^TOOLCHAIN_VERSION' | awk '{print $3'}`
            run mkdir -p $NDK/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI
            run mkdir -p $NDK/$GABIXX_SUBDIR/libs/$ABI
            run mkdir -p $NDK/$LIBPORTABLE_SUBDIR/libs/$ABI
            run gen_empty_archive $NDK/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI/libsupc++.a
            run gen_empty_archive $NDK/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI/libgnustl_static.a
            run gen_empty_bitcode $NDK/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI/libgnustl_shared.bc
            run gen_empty_archive $NDK/$GABIXX_SUBDIR/libs/$ABI/libgabi++_static.a
            run gen_empty_bitcode $NDK/$GABIXX_SUBDIR/libs/$ABI/libgabi++_shared.bc
            run cp -a $NDK/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$(get_default_abi_for_arch arm)/include $NDK/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI
        else
            echo "ERROR: Unsupported abi value: $ABI"
            exit 1
        fi
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
    if [ "$WINE" ]; then
        if [ "$WINE" = "wine12" ]; then
            run $WINE cmd /c Z:$NDK/ndk-build.cmd -j$JOBS "$@" APP_LDFLAGS="$APP_LDFLAGS" APP_CFLAGS="$APP_CFLAGS"
        else
            # do "clean" instead of -B
            run $WINE cmd /c Z:$NDK/ndk-build.cmd clean
            # make.exe can't do parallel build in wine > 1.2.x
            run $WINE cmd /c Z:$NDK/ndk-build.cmd "$@" -j1 APP_LDFLAGS="$APP_LDFLAGS" APP_CFLAGS="$APP_CFLAGS"
        fi
    else
        run $NDK/ndk-build -j$JOBS "$@" APP_LDFLAGS="$APP_LDFLAGS" APP_CFLAGS="$APP_CFLAGS"
    fi
}

# get build var
# $1: project directory
# $2: var
get_build_var ()
{
    local PROJECT=$1
    local VAR=$2

    if [ -z "$GNUMAKE" ] ; then
        GNUMAKE=make
    fi
    $GNUMAKE --no-print-dir -f $NDK/build/core/build-local.mk -C $PROJECT DUMP_$VAR | tail -1
}


# check if the project is broken and shouldn't be built
# $1: project directory
# $2: optional error message
is_broken_build ()
{
    local PROJECT="$1"
    local ERRMSG="$2"

    if [ -z "$RUN_TESTS" ] ; then
        if [ -f "$PROJECT/BROKEN_BUILD" ] ; then
            if [ ! -s "$PROJECT/BROKEN_BUILD" ] ; then
                # skip all
                if [ -z "$ERRMSG" ] ; then
                    echo "Skipping `basename $PROJECT`: (build)"
                else
                    echo "Skipping $ERRMSG: `basename $PROJECT`"
                fi
                return 0
            else
                # only skip listed in file
                TARGET_TOOLCHAIN=`get_build_var $PROJECT TARGET_TOOLCHAIN`
                TARGET_TOOLCHAIN_VERSION=`echo $TARGET_TOOLCHAIN | tr '-' '\n' | tail -1`
                grep -q -e "$TARGET_TOOLCHAIN_VERSION" "$PROJECT/BROKEN_BUILD"
                if [ $? = 0 ] ; then
                    if [ -z "$ERRMSG" ] ; then
                        echo "Skipping `basename $PROJECT`: (no build for $TARGET_TOOLCHAIN_VERSION)"
                    else
                        echo "Skipping $ERRMSG: `basename $PROJECT` (no build for $TARGET_TOOLCHAIN_VERSION)"
                    fi
                    return 0
                fi
                # skip incompatible forced platform
                if [ "$PLATFORM" != "" ] ; then
                    grep -q -e "$PLATFORM" "$PROJECT/BROKEN_BUILD" || grep -q -e "android-forced" "$PROJECT/BROKEN_BUILD"
                    if [ $? = 0 ] ; then
                        if [ -z "$ERRMSG" ] ; then
                            echo "Skipping `basename $PROJECT`: (no build for $PLATFORM)"
                        else
                            echo "Skipping $ERRMSG: `basename $PROJECT` (no build for $PLATFORM)"
                        fi
                        return 0
                    fi
                fi
            fi
        fi
    fi
    return 1
}

# check if $ABI is incompatible and shouldn't be built
# $1: project directory
is_incompatible_abi ()
{
    local PROJECT="$1"
    # Basically accept all for unknown arch, even some cases may not be suitable for this way
    if [ "$ABI" != "default" -a "$ABI" != "$(find_ndk_unknown_archs)" ] ; then
        # check APP_ABI
        local APP_ABIS=`get_build_var $PROJECT APP_ABI`
        APP_ABIS=$APP_ABIS" "
        if [ "$APP_ABIS" != "${APP_ABIS%%all*}" ] ; then
        # replace "all", "all32" and "all64"
          _EXPANDED=`get_build_var $PROJECT NDK_APP_ABI_ALL_EXPANDED`
          _FRONT="${APP_ABIS%%all*}"
          _BACK="${APP_ABIS#*all}"
          APP_ABIS="${_FRONT}${_EXPANDED}${_BACK}"
          _EXPANDED=`get_build_var $PROJECT NDK_APP_ABI_ALL32_EXPANDED`
          _FRONT="${APP_ABIS%%all32*}"
          _BACK="${APP_ABIS#*all32}"
          APP_ABIS="${_FRONT}${_EXPANDED}${_BACK}"
          _EXPANDED=`get_build_var $PROJECT NDK_APP_ABI_ALL64_EXPANDED`
          _FRONT="${APP_ABIS%%all64*}"
          _BACK="${APP_ABIS#*all64}"
          APP_ABIS="${_FRONT}${_EXPANDED}${_BACK}"
        fi
        if [ "$APP_ABIS" = "${APP_ABIS%$ABI *}" ] ; then
            echo "Skipping `basename $PROJECT`: incompatible ABI, needs $APP_ABIS"
            return 0
        fi
    fi
    return 1
}

compile_on_the_fly()
{
    local DSTDIR="$1"
    local COMPILER_PKGNAME="compiler.abcc"
    if [ -z "`$ADB_CMD -s "$DEVICE" shell pm path $COMPILER_PKGNAME`" ]; then
        dump "ERROR: No abcc found for unknown arch testing"
        return 1
    fi
    run $ADB_CMD -s "$DEVICE" shell am force-stop $COMPILER_PKGNAME
    run $ADB_CMD -s "$DEVICE" shell am startservice --user 0 -a ${COMPILER_PKGNAME}.BITCODE_COMPILE_TEST -n $COMPILER_PKGNAME/.AbccService -e working_dir $DSTDIR

    old_pid="`$ADB_CMD -s "$DEVICE" shell top -n 1 | grep $COMPILER_PKGNAME | awk '{print $1}'`"
    threshold=`echo $((60*10))` # Wait at most 10 minutes for large testcases
    sleep_seconds=0
    while [ 2 -eq 2 ]; do
      if [ $sleep_seconds -gt $threshold ]; then
        pid="`$ADB_CMD -s "$DEVICE" shell top -n 1 | grep $COMPILER_PKGNAME | awk '{print $1}'`"
        if [ "$pid" = "$old_pid" ]; then
          # Too much time
          break
        fi
        old_pid="$pid"
        sleep_seconds=0
      fi
      if [ -n "`$ADB_CMD -s "$DEVICE" shell ls $DSTDIR | grep compile_result`" ]; then
        # Compile done
        break
      fi
      sleep 3
      sleep_seconds="`echo $sleep_seconds + 3 | bc`"
    done
    ret="`$ADB_CMD -s "$DEVICE" shell cat $DSTDIR/compile_result`"
    ret=`echo $ret | tr -d "\r\n"`
    if [ $sleep_seconds -gt $threshold ] || [ "$ret" != "0" ]; then
      dump "ERROR: Could not compile bitcodes for $TEST_NAME on device"
      if [ $sleep_seconds -gt $threshold ]; then
        dump "- Reason: Compile time too long"
      elif [ -n "`$ADB_CMD -s "$DEVICE" shell ls $DSTDIR | grep compile_error`" ]; then
        dump "- Reason: `$ADB_CMD -s "$DEVICE" shell cat $DSTDIR/compile_error`"
      fi
      run $ADB_CMD -s "$DEVICE" shell am force-stop $COMPILER_PKGNAME
      return 1
    fi
    run $ADB_CMD -s "$DEVICE" shell am force-stop $COMPILER_PKGNAME
    return 0
}


build_project ()
{
    local NAME=`basename $1`
    local CHECK_ABI=$2
    local DIR="$BUILD_DIR/$NAME"

    if is_broken_build $1; then
        return 0;
    fi
    if [ "$CHECK_ABI" = "yes" ] ; then
        if is_incompatible_abi $1 ; then
            return 0
        fi
    fi
    rm -rf "$DIR" && cp -r "$1" "$DIR"
    # build it
    (run cd "$DIR" && run_ndk_build $NDK_BUILD_FLAGS)
    RET=$?
    if [ -f "$1/BUILD_SHOULD_FAIL" ]; then
        if [ $RET = 0 ]; then
            echo "!!! FAILURE: BUILD SHOULD HAVE FAILED [$1]"
            if [ "$CONTINUE_ON_BUILD_FAIL" != yes ] ; then
                exit 1
            fi
        fi
        log "!!! SUCCESS: BUILD FAILED AS EXPECTED [$(basename $1)]"
        RET=0
    fi
    if [ $RET != 0 ] ; then
        echo "!!! BUILD FAILURE [$1]!!! See $NDK_LOGFILE for details or use --verbose option!"
        if [ "$CONTINUE_ON_BUILD_FAIL" != yes ] ; then
            exit 1
        fi
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
            dump "Please clone platform/development.git from android.googlesource.com"
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
        build_project $1 "no"
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
        local NAME="$(basename $1)"
        echo "Building NDK build test: `basename $1`"
        if [ -f $1/build.sh ]; then
            local DIR="$BUILD_DIR/$NAME"
            if [ -f "$1/jni/Android.mk" -a -f "$1/jni/Application.mk" ] ; then
                # exclude jni/Android.mk with import-module because it needs NDK_MODULE_PATH
                grep -q  "call import-module" "$1/jni/Android.mk"
                if [ $? != 0 ] ; then
                    if (is_broken_build $1 || is_incompatible_abi $1) then
                        return 0;
                    fi
                fi
            fi
            rm -rf "$DIR" && cp -r "$1" "$DIR"
            export NDK
            (cd "$DIR" && run ./build.sh -j$JOBS $NDK_BUILD_FLAGS)
            if [ $? != 0 ]; then
                echo "!!! BUILD FAILURE [$1]!!! See $NDK_LOGFILE for details or use --verbose option!"
                if [ "$CONTINUE_ON_BUILD_FAIL" != yes ] ; then
                    exit 1
                fi
            fi
        else
            build_project $1 "yes"
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
###

CPU_ABIS=
if is_testable device; then
    build_device_test ()
    {
        if is_broken_build $1 "broken device test build"; then
            return 0;
        fi
        echo "Building NDK device test: `basename $1`"
        build_project $1 "yes"
    }

    # $1: DEVICE
    # $2: DEVICE CPU ABI
    # $3: test
    # $4: tmp dir
    run_device_test ()
    {
        local DEVICE=$1
        local CPU_ABI=$2
        local TEST=$3
        local TEST_NAME="$(basename $TEST)"
        local SRCDIR
        local DSTDIR="$4/$TARGET_TEST_SUBDIR"
        local SRCFILE
        local DSTFILE
        local PROGRAM
        # Do not run the test if BROKEN_RUN is defined
        if [ -z "$RUN_TESTS" ]; then
            if is_broken_build $TEST "NDK device test not built"; then
                return 0
            fi
            if [ -f "$TEST/BROKEN_RUN" ] ; then
                if [ ! -s "$TEST/BROKEN_RUN" ] ; then
                    # skip all
                    dump "Skipping NDK device test run: $TEST_NAME"
                    return 0
                else
                    # skip all tests built by toolchain
                    TARGET_TOOLCHAIN=`get_build_var $TEST TARGET_TOOLCHAIN`
                    TARGET_TOOLCHAIN_VERSION=`echo $TARGET_TOOLCHAIN | tr '-' '\n' | tail -1`
                    grep -q -e "$TARGET_TOOLCHAIN_VERSION" "$TEST/BROKEN_RUN"
                    if [ $? = 0 ] ; then
                        dump "Skipping NDK device test run: $TEST_NAME (no run for binary built by $TARGET_TOOLCHAIN_VERSION)"
                        return 0
                    fi
                    # skip tests listed in file
                    SKIPPED_EXECUTABLES=`cat $TEST/BROKEN_RUN | tr '\n' ' '`
                    dump "Skipping NDK device test run: $TEST_NAME ($SKIPPED_EXECUTABLES)"
                fi
            fi
        fi
        if [ "$ABI" = "$(find_ndk_unknown_archs)" ] && [ -d "$BUILD_DIR/`basename $TEST`/libs" ]; then
            cd $BUILD_DIR/`basename $TEST`/libs && cp -a $ABI $CPU_ABI
        fi
        SRCDIR="$BUILD_DIR/`basename $TEST`/libs/$CPU_ABI"
        if [ ! -d "$SRCDIR" ] || [ -z "`ls $SRCDIR`" ]; then
            dump "Skipping NDK device test run (no $CPU_ABI binaries): $TEST_NAME"
            return 0
        fi
        # First, copy all files to the device, except for gdbserver, gdb.setup, and
        # those declared in $TEST/BROKEN_RUN
        adb_shell_mkdir "$DEVICE" $DSTDIR

        if [ "$ABI" = "$(find_ndk_unknown_archs)" ]; then # on-the-fly on-device compilation
            run $ADB_CMD -s "$DEVICE" shell rm -rf $DSTDIR/abcc_tmp
            adb_shell_mkdir "$DEVICE" $DSTDIR/abcc_tmp
            run $ADB_CMD -s "$DEVICE" shell chmod 0777 $DSTDIR/abcc_tmp
            for SRCFILE in `ls $SRCDIR`; do
                run $ADB_CMD -s "$DEVICE" push "$SRCDIR/$SRCFILE" $DSTDIR/abcc_tmp
                run $ADB_CMD -s "$DEVICE" shell chmod 0644 $DSTDIR/abcc_tmp/$SRCFILE
            done
            compile_on_the_fly $DSTDIR/abcc_tmp
            if [ $? -ne 0 ]; then
                test "$CONTINUE_ON_BUILD_FAIL" != "yes" && exit 1
                return 1
            fi
            run rm -f $SRCDIR/*
            run $ADB_CMD -s "$DEVICE" pull $DSTDIR/abcc_tmp $SRCDIR
            run rm -f $SRCDIR/compile_result
            run rm -f $SRCDIR/compile_error
            run rm -f $SRCDIR/*$(get_lib_suffix_for_abi $ABI)
            run $ADB_CMD -s "$DEVICE" shell rm -rf $DSTDIR/abcc_tmp
        fi

        for SRCFILE in `ls $SRCDIR`; do
            DSTFILE=`basename $SRCFILE`
            echo "$DSTFILE" | grep -q -e '\.so$'
            if [ $? != 0 ] ; then
                continue
            fi
            SRCPATH="$SRCDIR/$SRCFILE"
            if [ $HOST_OS = cygwin ]; then
                SRCPATH=`cygpath -m $SRCPATH`
            fi
            DSTPATH="$DSTDIR/$DSTFILE"
            run $ADB_CMD -s "$DEVICE" push "$SRCPATH" "$DSTPATH" &&
            run $ADB_CMD -s "$DEVICE" shell chmod 0755 $DSTPATH
            if [ $? != 0 ] ; then
                dump "ERROR: Could not install $SRCPATH to device $DEVICE!"
                exit 1
            fi
        done

        for SRCFILE in `ls $SRCDIR`; do
            DSTFILE=`basename $SRCFILE`
            if [ "$DSTFILE" = "gdbserver" -o "$DSTFILE" = "gdb.setup" ] ; then
                continue
            fi
            echo "$DSTFILE" | grep -q -e '\.so$'
            if [ $? = 0 ] ; then
              continue
            fi
            if [ -z "$RUN_TESTS" -o "$RUN_TESTS_FILTERED" = "yes" ]; then
                if [ -f "$TEST/BROKEN_RUN" ]; then
                    grep -q -w -e "$DSTFILE" "$TEST/BROKEN_RUN"
                    if [ $? = 0 ] ; then
                        continue
                    fi
                fi
            fi
            SRCPATH="$SRCDIR/$SRCFILE"
            if [ $HOST_OS = cygwin ]; then
                SRCPATH=`cygpath -m $SRCPATH`
            fi
            DSTPATH="$DSTDIR/$DSTFILE"
            run $ADB_CMD -s "$DEVICE" push "$SRCPATH" "$DSTPATH" &&
            run $ADB_CMD -s "$DEVICE" shell chmod 0755 $DSTPATH
            DATAPATHS=
            if [ -f "$TEST/DATA" ]; then
                if grep -q -e "$DSTFILE" "$TEST/DATA"; then
                    DATAPATHS=`grep -e "$DSTFILE" "$TEST/DATA" | awk '{print $2}'`
                    DATAPATHS=$NDK/$DATAPATHS
                    for DATA in $(ls $DATAPATHS); do
                        run $ADB_CMD -s "$DEVICE" push "$DATA" "$DSTDIR"
                    done
                fi
            fi
            if [ $? != 0 ] ; then
                dump "ERROR: Could not install $SRCPATH to device $DEVICE!"
                exit 1
            fi
            PROGRAM="`basename $DSTPATH`"
            dump "Running device test [$CPU_ABI]: $TEST_NAME (`basename $PROGRAM`)"
            adb_var_shell_cmd "$DEVICE" "" "cd $DSTDIR && LD_LIBRARY_PATH=$DSTDIR ./$PROGRAM"
            if [ $? != 0 ] ; then
                dump "   ---> TEST FAILED!!"
            fi
            adb_var_shell_cmd "$DEVICE" "" "rm -f $DSTPATH"
            if [ -n "$DATAPATHS" ]; then
                for DATA in $(ls $DATAPATHS); do
                    adb_var_shell_cmd "$DEVICE" "" "rm -f $DSTDIR/`basename $DATA`"
                done
            fi
        done
        # Cleanup
        adb_var_shell_cmd "$DEVICE" "" rm -r $DSTDIR
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
        # Get list of online devices, turn ' ' in device into '#'
        ADB_DEVICES=`$ADB_CMD devices | grep -v offline | awk 'NR>1 {gsub(/[ \t]+device$/,""); print;}' | sed '/^$/d' | sort | tr ' ' '#'`
        ADB_DEVICES=$(echo $ADB_DEVICES | tr '\n' ' ')
        log2 "ADB online devices (sorted): $ADB_DEVICES"
        ADB_DEVCOUNT=`echo "$ADB_DEVICES" | wc -w`
        if [ "$ADB_DEVCOUNT" = "0" ]; then
            dump "WARNING: No device connected to adb!"
            SKIP_TESTS=yes
        else
            ADB_DEVICES="$ADB_DEVICES "
            if [ -n "$ANDROID_SERIAL" ] ; then
                # Expect ANDROID_SERIAL is comma-delimited of one or more devices
                ANDROID_SERIAL=$(echo "$ANDROID_SERIAL" | tr ' ' '#')  # turn ' ' into '#'
                ANDROID_SERIAL=$(commas_to_spaces $ANDROID_SERIAL)
                for SERIAL in $ANDROID_SERIAL; do
                    if [ "$ADB_DEVICES" = "${ADB_DEVICES%$SERIAL *}" ] ; then
                        dump "WARNING: Device $SERIAL cannot be found or offline!"
                        SKIP_TESTS=yes
                    fi
                done
                if [ "$SKIP_TESTS" != "yes" ] ; then
                    ADB_DEVICES="$ANDROID_SERIAL"
                fi
            fi
        fi
    fi
    if [ "$SKIP_TESTS" = "yes" ] ; then
        dump "SKIPPING RUNNING TESTS ON DEVICE!"
    else
        AT_LEAST_CPU_ABI_MATCH=
        for DEVICE in $ADB_DEVICES; do
            # undo earlier ' '-to-'#' translation
            DEVICE=$(echo "$DEVICE" | tr '#' ' ')
            # get device CPU_ABI and CPU_ABI2, each may contain list of abi, comma-delimited.
            adb_var_shell_cmd "$DEVICE" CPU_ABI1 getprop ro.product.cpu.abi
            adb_var_shell_cmd "$DEVICE" CPU_ABI2 getprop ro.product.cpu.abi2
            CPU_ABIS="$CPU_ABI1,$CPU_ABI2"
            CPU_ABIS=$(commas_to_spaces $CPU_ABIS)
            if [ -n "$_NDK_TESTING_ALL_" ]; then
                if [ "$CPU_ABI1" = "armeabi-v7a" -o "$CPU_ABI2" = "armeabi-v7a" ]; then
                    CPU_ABIS="$CPU_ABIS armeabi-v7a-hard"
                fi
            fi
            if [ "$CPU_ABIS" = " " ]; then
              # Very old cupcake-based Android devices don't have these properties
              # defined. Fortunately, they are all armeabi-based.
              CPU_ABIS=armeabi
            fi
            log "CPU_ABIS=$CPU_ABIS"
            for CPU_ABI in $CPU_ABIS; do
                if [ "$ABI" = "default" -o "$ABI" = "$CPU_ABI" -o "$ABI" = "$(find_ndk_unknown_archs)" ] ; then
                    AT_LEAST_CPU_ABI_MATCH="yes"
                    for DIR in `ls -d $ROOTDIR/tests/device/*`; do
                        if is_buildable $DIR; then
                            log "Running device test on $DEVICE [$CPU_ABI]: $DIR"
                            run_device_test "$DEVICE" "$CPU_ABI" "$DIR" /data/local/tmp
                        fi
                    done
                fi
            done
        done
        if [ "$AT_LEAST_CPU_ABI_MATCH" != "yes" ] ; then
            dump "WARNING: No device matches ABI $ABI! SKIPPING RUNNING TESTS ON DEVICE!"
        fi
    fi
fi

dump "Cleaning up..."
if [ "$ABI" = "$(find_ndk_unknown_archs)" ]; then
  # Cleanup some intermediate files for testing
  run rm -rf $NDK/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI
  run rm -rf $NDK/$GABIXX_SUBDIR/libs/$ABI
  run rm -rf $NDK/$LIBPORTABLE_SUBDIR/libs/$ABI
fi
rm -rf $BUILD_DIR
dump "Done."
