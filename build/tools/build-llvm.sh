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
#  This shell script is used to rebuild the llvm and clang binaries
#  for the Android NDK.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <ndk-dir> <toolchain>"

PROGRAM_DESCRIPTION=\
"Rebuild the LLVM prebuilt binaries for the Android NDK.

Where <src-dir> is the location of toolchain sources, <ndk-dir> is
the top-level NDK installation path and <toolchain> is the name of
the toolchain to use (e.g. llvm-3.1)."

RELEASE=`date +%Y%m%d`
BUILD_OUT=/tmp/ndk-$USER/build/toolchain
OPTION_BUILD_OUT=
register_var_option "--build-out=<path>" OPTION_BUILD_OUT "Set temporary build directory"

# Note: platform API level 9 or higher is needed for proper C++ support
PLATFORM=$DEFAULT_PLATFORM
register_var_option "--platform=<name>"  PLATFORM "Specify platform name"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Create archive tarball in specific directory"

register_jobs_option
register_mingw_option
register_try64_option

extract_parameters "$@"

prepare_mingw_toolchain /tmp/ndk-$USER/build

fix_option BUILD_OUT "$OPTION_BUILD_OUT" "build directory"
setup_default_log_file $BUILD_OUT/config.log

set_parameters ()
{
    SRC_DIR="$1"
    NDK_DIR="$2"
    TOOLCHAIN="$3"

    # Check source directory
    #
    if [ -z "$SRC_DIR" ] ; then
        echo "ERROR: Missing source directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$SRC_DIR/llvm" ] ; then
        echo "ERROR: Source directory does not contain llvm sources: $SRC_DIR"
        exit 1
    fi

    log "Using source directory: $SRC_DIR"

    # Check NDK installation directory
    #
    if [ -z "$NDK_DIR" ] ; then
        echo "ERROR: Missing NDK directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$NDK_DIR" ] ; then
        mkdir -p $NDK_DIR
        if [ $? != 0 ] ; then
            echo "ERROR: Could not create target NDK installation path: $NDK_DIR"
            exit 1
        fi
    fi

    log "Using NDK directory: $NDK_DIR"

    # Check toolchain name
    #
    if [ -z "$TOOLCHAIN" ] ; then
        echo "ERROR: Missing toolchain name parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$SRC_DIR/llvm/$TOOLCHAIN" ] ; then
        echo "ERROR: Source directory does not contain llvm sources for $TOOLCHAIN"
    fi
}

set_parameters $PARAMETERS

prepare_target_build

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
fi

set_toolchain_ndk $NDK_DIR $TOOLCHAIN

dump "Using C compiler: $CC"
dump "Using C++ compiler: $CXX"

rm -rf $BUILD_OUT
mkdir -p $BUILD_OUT

TOOLCHAIN_BUILD_PREFIX=$BUILD_OUT/prefix

# configure the toolchain
dump "Configure: $TOOLCHAIN toolchain build"
cd $BUILD_OUT
export CC CXX CFLAGS CXXFLAGS
run $SRC_DIR/llvm/$TOOLCHAIN/configure \
    --prefix=$TOOLCHAIN_BUILD_PREFIX \
    --host=$ABI_CONFIGURE_HOST \
    --build=$ABI_CONFIGURE_BUILD \
    --enable-targets=arm,mips,x86 \
    $CONFIG_FLAGS
fail_panic "Couldn't configure llvm toolchain"


# build the toolchain
dump "Building : llvm toolchain [this can take a long time]."
cd $BUILD_OUT
export CC CXX CFLAGS CXXFLAGS
run make -j$NUM_JOBS
fail_panic "Couldn't compile llvm toolchain"

# install the toolchain to its final location
dump "Install  : llvm toolchain binaries."
cd $BUILD_OUT && run make install
fail_panic "Couldn't install llvm toolchain to $TOOLCHAIN_BUILD_PREFIX"

# clean static or shared libraries
rm -rf $TOOLCHAIN_BUILD_PREFIX/docs
rm -rf $TOOLCHAIN_BUILD_PREFIX/include
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/*.a
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/*.so
rm -rf $TOOLCHAIN_BUILD_PREFIX/share

UNUSED_LLVM_EXECUTABLES="
bugpoint c-index-test clang-tblgen lli llvm-ar llvm-as llvm-bcanalyzer
llvm-config llvm-cov llvm-diff llvm-dwarfdump llvm-extract llvm-ld llvm-mc
llvm-nm llvm-objdump llvm-prof llvm-ranlib llvm-readobj llvm-rtdyld llvm-size
llvm-stress llvm-stub llvm-tblgen macho-dump"

for i in $UNUSED_LLVM_EXECUTABLES; do
    rm -f $TOOLCHAIN_BUILD_PREFIX/bin/$i
    rm -f $TOOLCHAIN_BUILD_PREFIX/bin/$i.exe
done

# copy to toolchain path
run copy_directory "$TOOLCHAIN_BUILD_PREFIX" "$TOOLCHAIN_PATH"

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE="$TOOLCHAIN-$HOST_TAG.tar.bz2"
    SUBDIR=$(get_toolchain_install_subdir $TOOLCHAIN $HOST_TAG)
    dump "Packaging $ARCHIVE"
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SUBDIR"
fi

dump "Done."
if [ -z "$OPTION_BUILD_OUT" ] ; then
    rm -rf $BUILD_OUT
fi
