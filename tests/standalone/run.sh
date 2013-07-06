#!/bin/sh
#
# Copyright (C) 2012 The Android Open Source Project
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

# This script is used to run a series of tests on a given standalone
# toolchain. You need to define the following variables before calling it:
#
#   PREFIX    Full binary prefix to the toolchain binaries,
#             e.g. '/path/to/toolchain/bin/arm-linux-androideabi-'
#             This script will use ${PREFIX}gcc to invoke the compiler,
#             ${PREFIX}ar for the archiver, etc...
#
#   CFLAGS    Compiler flags for C programs.
#   CXXFLAGS  Compiler flags for C++ programs.
#   LDFLAGS   Linker flags (passed to ${PREFIX}gcc, not ${PREFIX}ld)
#

PROGNAME=$(basename "$0")
PROGDIR=$(dirname "$0")
NDK_ROOT=$(cd "$PROGDIR/../.." && pwd)
. $NDK_ROOT/build/tools/ndk-common.sh

panic () {
    echo "ERROR: $@" >&2; exit 1
}

fail_panic () {
  if [ $? != 0 ]; then panic "$@"; fi
}


# Command-line processing
#
# Note: try to keep in alphabetical order, same for the --option cases below.
#
ABI=
HELP=
LIST_TESTS=
NO_SYSROOT=
SYSROOT=
TEST_SUBDIRS=
VERBOSE=1

# Parse options
for opt; do
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case $opt in
        --abi=*)
            ABI=$optarg
            ;;
        --help|-h|-?)
            HELP=true
            ;;
        --list)
            LIST_TESTS=true
            ;;
        --no-sysroot)
            NO_SYSROOT=true
            ;;
        --prefix=*)
            PREFIX=$optarg
            ;;
        --quiet|-q)
            VERBOSE=$(( $VERBOSE - 1 ))
            ;;
        --sysroot=*)
            SYSROOT=$optarg
            ;;
        --verbose|-v)
            VERBOSE=$(( $VERBOSE + 1 ))
            ;;
        -*)
            panic "Unknown option '$opt'. See --help for list of valid ones."
            ;;
        *)
            TEST_SUBDIRS=$TEST_SUBDIRS" "$opt
            ;;
    esac
done

if [ "$HELP" ]; then
    echo "Usage: $PROGNAME [options] [testname+]"
    echo ""
    echo "Run a set of unit tests to check that a given Android NDK toolchain works"
    echo "as expected. Useful to catch regressions when generating new toolchain"
    echo "binaries."
    echo ""
    echo "You can pass the full path to the toolchain either with the --prefix"
    echo "option, or by defining PREFIX in your environment before calling this script."
    echo "For example:"
    echo ""
    echo "  $PROGNAME --prefix=\$NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/bin/arm-linux-androideabi"
    echo ""
    echo "The prefix can also be the full-path to the \$TARGET-gcc or \$TARGET-g++ program "
    echo ""
    echo "The script will automatically use an NDK-provided sysroot, but you can specify an"
    echo "alternate one with the --sysroot=<path> option. You can also use --no-sysroot if"
    echo "the toolchain already provides its own sysroot (e.g. if it is a standalone toolchain"
    echo "generated with make-standalone-toolchain.sh)."
    echo ""
    echo "The target ABI is normally auto-detected from the toolchain, but you can specify an"
    echo "alternative one with the --abi=<name> option. This is only useful on ARM, where the"
    echo "default ABI is 'armeabi' targetting the ARMv5TE instruction set. If you want to check"
    echo "the generation of ARMv7-A machine code, use the following:"
    echo ""
    echo "  --abi=armeabi-v7a"
    echo ""
    echo "When called without any arguments, the script will run all known toolchain tests."
    echo "You can restrict the list of tests by passing them on the command-line. Use --list"
    echo "to display the list of all tests that are relevant for your current ABI."
    echo ""
    echo "More information about each test can be displayed by using --verbose."
    echo ""
    echo "Valid options:"
    echo ""
    echo "    --help|-h|-?        Print this message."
    echo "    --verbose|-v        Increase verbosity."
    echo "    --quiet|-q          Decrease verbosity."
    echo "    --list              List all available tests for current ABI."
    echo "    --prefix=<prefix>   Specify full toolchain binary prefix."
    echo "    --sysroot=<path>    Specify alternate sysroot."
    echo "    --no-sysroot        Do not use a sysroot."
    echo "    --abi=<name>        Specify target ABI name."
    echo ""
    exit 0
fi

TMPDIR=/tmp/ndk-$USER/tests/standalone
mkdir -p "$TMPDIR" && rm -rf "$TMPDIR/*"

BUILD_DIR=$TMPDIR/build
mkdir -p "$BUILD_DIR"

LOGFILE=$TMPDIR/log.txt
echo -n "" > $LOGFILE

if [ $VERBOSE -ge 3 ]; then
    run () {
        echo "# COMMAND: $@"
        "$@"
    }
elif [ $VERBOSE -ge 2 ]; then
    run () {
        echo "# COMMAND: $@" >> $LOGFILE
        "$@"
    }
else
    run () {
        echo "# COMMAND[$@]" >> $LOGFILE
        "$@" >> $LOGFILE 2>&1
    }
fi

if [ $VERBOSE -ge 2 ]; then
    run_script () {
        $SHELL "$@"
    }
else
    run_script () {
        $SHELL "$@" >> $LOGFILE 2>&1
    }
fi

if [ $VERBOSE -ge 1 ]; then
    dump () {
        echo "$@"
    }
else
    dump () {
        :  # nothing
    }
fi

if [ "$HOST_OS" = "cygwin" -o "$HOST_OS" = "windows" ] ; then
    NULL="NUL"
else
    NULL="/dev/null"
fi

# Probe a given sub-directory and see if it contains valid test files.
# $1: sub-directory path
# Return: 0 on success, 1 otherwise
#
# This can also sets the following global variables:
#   TEST_TYPE
#   SCRIPT
#   SOURCES
#
probe_test_subdir ()
{
    local DIR="$1"

    TEST_TYPE=
    SCRIPT=
    SOURCES=

    if [ -f "$DIR/run.sh" ]; then
        TEST_TYPE=script
        SCRIPT=run.sh

    elif [ -f "$DIR/run-$ABI.sh" ]; then
        TEST_TYPE=script
        SCRIPT=run-$ABI.sh

    elif [ -f "$DIR/main.c" ]; then
        TEST_TYPE=c_executable
        SOURCES=main.c

    elif [ -f "$DIR/main.cpp" ]; then
        TEST_TYPE=cxx_executable
        SOURCES=main.cpp

    else
        return 1
    fi

    return 0
}


# Handle --list option now, then exit
if [ -n "$LIST_TESTS" ]; then
    echo "List of available toolchain tests:"
    if [ -z "$ABI" ]; then
        ABI=armeabi
    fi
    for TEST_SUBDIR in $(cd $PROGDIR && ls -d *); do
        SUBDIR=$PROGDIR/$TEST_SUBDIR
        if probe_test_subdir "$SUBDIR"; then
            echo "  $TEST_SUBDIR"
        fi
    done
    exit 0
fi

if [ -z "$PREFIX" ]; then
    panic "Please define PREFIX in your environment, or use --prefix=<prefix> option."
fi

CC=
CXX=
CC_TARGET=
if [ "$PREFIX" = "${PREFIX%clang}" ]; then
    # Test GCC
    # Remove -gcc or -g++ from prefix if any
    PREFIX=${PREFIX%-gcc}
    PREFIX=${PREFIX%-g++}

    # Add a trailing dash to the prefix, if there isn't any
    PREFIX=${PREFIX%-}-

    GCC=${PREFIX}gcc
    if [ ! -f "$GCC" ]; then
        panic "Missing compiler, please fix your prefix definition: $GCC"
    fi

    GCC=$(which $GCC 2>$NULL)
    if [ -z "$GCC" -o ! -f "$GCC" ]; then
        panic "Bad compiler path: ${PREFIX}gcc"
    fi

    # Remove trailing .exe if any
    GCC=${GCC%${HOST_EXE}}

    GCCDIR=$(dirname "$GCC")
    GCCBASE=$(basename "$GCC")

    GCCDIR=$(cd "$GCCDIR" && pwd)
    GCC=$GCCDIR/$GCCBASE

    PREFIX=${GCC%%gcc}

    CC=${PREFIX}gcc
    CXX=${PREFIX}g++
    CC_TARGET=$($GCC -v 2>&1 | tr ' ' '\n' | grep -e --target=)
    CC_TARGET=${CC_TARGET##--target=}
else
    # Test Clang
    # Remove clang or clang++ from prefix if any
    PREFIX=${PREFIX%clang}
    PREFIX=${PREFIX%clang++}

    CLANG=${PREFIX}clang
    if [ ! -f "$CLANG" ]; then
        panic "Missing compiler, please fix your prefix definition: $CLANG"
    fi

    CLANGDIR=$(dirname "$CLANG")
    CLANGBASE=$(basename "$CLANG")

    CLANGDIR=$(cd "$CLANGDIR" && pwd)
    CLANG=$CLANGDIR/$CLANGBASE

    PREFIX=${CLANG%%clang}

    # Find *-ld in the same directory eventaully usable as ${PREFIX}-ld
    GNU_LD=$(cd $CLANGDIR && ls *-ld${HOST_EXE})
    GNU_LD=$CLANGDIR/$GNU_LD
    if [ ! -f "$GNU_LD" ]; then
        panic "Missing linker in the same directory as clang/clang++: $CLANGDIR"
    fi

    PREFIX=${GNU_LD%ld${HOST_EXE}}

    CC=$CLANG
    CXX=${CLANG%clang}clang++
    CC_TARGET=$($CLANG -v 2>&1 | grep Target:)
    CC_TARGET=${CC_TARGET##Target: }
fi

if [ -z "$ABI" ]; then
    # Auto-detect target CPU architecture
    dump "Auto-detected target configuration: $CC_TARGET"
    case $CC_TARGET in
        arm*-linux-androideabi)
            ABI=armeabi
            ARCH=arm
            ;;
        i686*-linux-android)
            ABI=x86
            ARCH=x86
            ;;
        mipsel*-linux-android)
            ABI=mips
            ARCH=mips
            ;;
        *)
            panic "Unknown target architecture '$CC_TARGET', please use --abi=<name> to manually specify ABI."
    esac
    dump "Auto-config: --abi=$ABI"
fi

COMMON_FLAGS=

# Ensure ABI_<abi> is defined as a compiler macro when building test programs.
# as a compiler macro when building all test programs.
ABI_MACRO=ABI_$(echo "$ABI" | tr '-' '_')
COMMON_FLAGS=$COMMON_FLAGS" -D$ABI_MACRO=1"

if [ -n "$NO_SYSROOT" ]; then
    SYSROOT=
elif [ -n "$SYSROOT" ]; then
    if [ ! -d "$SYSROOT" ]; then
        panic "Sysroot directory does not exist: $SYSROOT"
    fi
    # Sysroot must be absolute path
    SYSROOT=$(cd $SYSROOT && pwd)
    COMMON_FLAGS=$COMMON_FLAGS" --sysroot=$SYSROOT"
else
    # Auto-detect sysroot
    SYSROOT=$NDK_ROOT/platforms/android-9/arch-$ARCH
    if [ ! -d "$SYSROOT" ]; then
        panic "Can't find sysroot file, use --sysroot to point to valid one: $SYSROOT"
    fi
    if [ ! -f "$SYSROOT/usr/lib/libc.so" ]; then
        panic "Incomplete sysroot, use --sysroot to point to valid one: $SYSROOT"
    fi
    if [ "$HOST_OS" = "cygwin" ]; then
        SYSROOT=`cygpath -m $SYSROOT`
    else
        if [ "$HOST_OS" = "windows" -a "$OSTYPE" = "msys" ]; then
            # use -W specific to MSys to get windows path
            SYSROOT=$(cd $SYSROOT ; pwd -W)
        fi
    fi
    dump "Auto-config: --sysroot=$SYSROOT"
    COMMON_FLAGS=$COMMON_FLAGS" --sysroot=$SYSROOT"
fi

if [ -z "$CXXFLAGS" ]; then
    CXXFLAGS=$CFLAGS
fi

# NOTE: We need to add -fno-exceptions, otherwise some toolchains compile
#        with exception support by default, and the test programs fail to
#        link due to an undefined reference to __gxx_personality_v0.
#
#        This symbol is normally part of libsupc++ which is not available
#        if you don't have the GNU libstdc++ installed into your toolchain
#        directory.
#
#        Affects the x86 and mips toolchains, but not the ARM one.
#        Not sure if we want exceptions enabled by default or not.
#
CXXFLAGS=$CXXFLAGS" -fno-exceptions"

CFLAGS=$COMMON_FLAGS" "$CFLAGS
CXXFLAGS=$COMMON_FLAGS" "$CXXFLAGS

if [ -z "$TEST_SUBDIRS" ]; then
    TEST_SUBDIRS=$(cd $PROGDIR && ls -d *)
fi

COUNT=0
FAILURES=0
for TEST_SUBDIR in $TEST_SUBDIRS; do
    SUBDIR=$PROGDIR/$TEST_SUBDIR

    if ! probe_test_subdir "$SUBDIR"; then
        continue
    fi

    rm -rf "$BUILD_DIR"/* &&
    cp -RL "$SUBDIR"/* "$BUILD_DIR/"
    fail_panic "Could not copy test files to $BUILD_DIR !?"

    dump_n "Running $TEST_SUBDIR test... "

    case $TEST_TYPE in
        script)
            (
                export PREFIX CC CXX CFLAGS CXXFLAGS LDFLAGS VERBOSE ABI NULL
                run cd "$BUILD_DIR" && run_script $SCRIPT
            )
            RET=$?
            ;;

        c_executable)
            (
                run cd "$BUILD_DIR" && run $CC $LDFLAGS $CFLAGS -o $NULL $SOURCES
            )
            RET=$?
            ;;

        cxx_executable)
            (
                run cd "$BUILD_DIR" && run $CXX $LDFLAGS $CXXFLAGS -o $NULL $SOURCES
            )
            RET=$?
            ;;
    esac

    if [ "$RET" != 0 ]; then
        dump "KO"
        FAILURES=$(( $FAILURES + 1 ))
    else
        dump "ok"
    fi
    COUNT=$(( $COUNT + 1 ))
done

if [ "$FAILURES" -eq 0 ]; then
    dump "$COUNT/$COUNT tests passed. Success."
    exit 0
else
    dump "$FAILURES tests failed out of $COUNT."
    exit 1
fi
