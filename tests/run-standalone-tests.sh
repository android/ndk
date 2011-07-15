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

# Extract current directory
PROGDIR=$(dirname $0)
PROGDIR=$(cd $PROGDIR && pwd)
PROGNAME=$(basename $0)

# Compute NDK directory, we assume we are under tests/
NDK=$(dirname $PROGDIR)

# Prepare the temporary standalone toolchain
ROOTDIR=/tmp/ndk-$USER/tests/standalone

PARAMETERS=
OPTION_HELP=
VERBOSE=0
while [ -n "$1" ]; do
    opt="$1"
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case "$opt" in
        --help|-h|-\?)
            OPTION_HELP=yes
            ;;
        --verbose)
            VERBOSE=$(( $VERBOSE + 1 ))
            ;;
        --arch=*)
            ARCH=$optarg
            ;;
        --platform=*)
            PLATFORM=$optarg
            PLATFORM=${PLATFORM##android-}
            PLATFORM=android-$PLATFORM
            ;;
        -*) # unknown options
            echo "ERROR: Unknown option '$opt', use --help for list of valid ones."
            exit 1
        ;;
        *)  # Simply record parameter
            if [ -z "$PARAMETERS" ] ; then
                PARAMETERS=$opt
            else
                PARAMETERS=$PARAMETERS" $opt"
            fi
            ;;
    esac
    shift
done

if [ "$OPTION_HELP" ] ; then
    echo "Usage: $PROGNAME [options]"
    echo ""
    echo "Run the standalone toolchain tests."
    echo ""
    echo "Valid options:"
    echo ""
    echo "    --help|-h|-?        Print this help"
    echo "    --verbose           Increment verbosity (can be used several times)"
    echo "    --arch=<name>       Specify architecture [arm]"
    echo "    --platform=<level>  Specify target platform [android-9]"
    echo ""
    exit 0
fi


# Where we're going to place generated files
OBJDIR=$ROOTDIR/obj
mkdir -p $OBJDIR

get_default_toolchain_prefix_for ()
{
    case $1 in
    arm)
        echo "arm-linux-androideabi"
        ;;
    x86)
        echo "i686-android-linux"
        ;;
    *)
        echo "unknown-toolchain"
        ;;
    esac
}

# Install standalone toolchain
# $1: API level (e.g. android-3)
# $2: Architecture name
# This sets TOOLCHAINDIR and TOOLCHAINPREFIX
install_toolchain ()
{
    local LEVEL=`echo $1 | sed -e 's!android-!!g'`
    TOOLCHAINDIR=$ROOTDIR/toolchain-$LEVEL
    mkdir -p $TOOLCHAINDIR
    echo "Installing $ARCH standalone toolchain into: $TOOLCHAINDIR"
    $NDK/build/tools/make-standalone-toolchain.sh --install-dir=$TOOLCHAINDIR --platform=$1 --arch=$2
    if [ $? != 0 ] ; then
        echo "ERROR: Could not install toolchain for platform $1."
        exit 1
    fi
    # XXX: TODO: Extract dynamically
    TOOLCHAINPREFIX=$TOOLCHAINDIR/bin/$(get_default_toolchain_prefix_for $ARCH)
}

# Compile and link a C++ source file
compile_and_link_cxx ()
{
    local EXENAME="$(basename $1)"
    EXENAME=${EXENAME##.c}
    EXENAME=${EXENAME##.cpp}
    echo "Building C++ test: $EXENAME"
    $TOOLCHAINPREFIX-g++ -o $OBJDIR/$EXENAME $1
    if [ $? != 0 ] ; then
        echo "ERROR: Could not compile C++ source file: $1"
        exit 2
    fi
}

install_toolchain android-9 $ARCH

for CXXSRC in $PROGDIR/standalone/*/*.cpp $PROGDIR/standalone/*/*.c; do
    compile_and_link_cxx $CXXSRC
done

echo "Cleaning up."
rm -rf $ROOTDIR
