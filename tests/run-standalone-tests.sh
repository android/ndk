#!/bin/sh
#

# Extract current directory
PROGDIR=`dirname $0`
PROGDIR=`cd $PROGDIR && pwd`

# Compute NDK directory
NDK=`dirname $PROGDIR`

# Prepare the standalone toolchain
ROOTDIR=/tmp/ndk/standalone

# Where we're going to place generated files
OBJDIR=$ROOTDIR/obj
mkdir -p $OBJDIR

# Install standalone toolchain
# $1: API level (e.g. android-3)
# This sets TOOLCHAINDIR and TOOLCHAINPREFIX
install_toolchain ()
{
    local LEVEL=`echo $1 | sed -e 's!android-!!g'`
    TOOLCHAINDIR=$ROOTDIR/toolchain-$LEVEL
    mkdir -p $TOOLCHAINDIR
    $NDK/build/tools/make-standalone-toolchain.sh --install-dir=$TOOLCHAINDIR --platform=$1
    if [ $? != 0 ] ; then
        echo "ERROR: Could not install toolchain for platform $1."
        exit 1
    fi
    # XXX: TODO: Extract dynamically
    TOOLCHAINPREFIX=$TOOLCHAINDIR/bin/arm-linux-androideabi
}

# Compile and link a C++ source file
compile_and_link_cxx ()
{
    local EXENAME=`basename $1 | sed -e 's!\.cpp!!g'`
    echo "Building C++ test: $EXENAME"
    $TOOLCHAINPREFIX-g++ -o $OBJDIR/$EXENAME $1
    if [ $? != 0 ] ; then
        echo "ERROR: Could not compile C++ source file: $1"
        exit 2
    fi
}

install_toolchain android-9

for CXXSRC in $PROGDIR/standalone/*/*.cpp; do
    compile_and_link_cxx $CXXSRC
done

echo "Cleaning up."
rm -rf $ROOTDIR
