#!/bin/sh
#
# Copyright (C) 2013 The Android Open Source Project
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
#  This shell script is used to rebuild the on-device llvm and mclinker
#  binaries for the Android NDK.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <ndk-dir>"

PROGRAM_DESCRIPTION=\
"Rebuild LLVM-related on-device binaries for the Android NDK.

Where <src-dir> is the location of toolchain sources, <ndk-dir> is
the top-level NDK installation path."

RELEASE=`date +%Y%m%d`
BUILD_OUT=/tmp/ndk-$USER/build/toolchain
TOOLCHAIN_BUILD_PREFIX="$BUILD_OUT/../prefix"
ARCHS="$DEFAULT_ARCHS"
BUILDTOOLS=$ANDROID_NDK_ROOT/build/tools

OPTION_OUT_DIR=
register_var_option "--out-dir=<path>" OPTION_OUT_DIR "On-device toolchain will be put at <path>"

OPTION_ARCH=
register_var_option "--arch=<arm,x86,mips>" OPTION_ARCH "Default: all"

OPTION_GCC_VERSION=
register_var_option "--gcc-version=<version>" OPTION_GCC_VERSION "Specify GCC toolchain version [Default: $DEFAULT_GCC_VERSION]"

register_jobs_option

extract_parameters "$@"

prepare_canadian_toolchain /tmp/ndk-$USER/build

fix_option TOOLCHAIN_BUILD_PREFIX "$OPTION_OUT_DIR" "On-device toolchain will be put at <path>"
fix_option ARCHS "$OPTION_ARCH" "on-device architecture"
ARCHS="`commas_to_spaces $ARCHS`"
setup_default_log_file $BUILD_OUT/config.log

set_parameters ()
{
    SRC_DIR="$1"
    NDK_DIR="$2"
    TOOLCHAIN="llvm-$DEFAULT_LLVM_VERSION"

    # Check source directory
    #
    if [ -z "$SRC_DIR" ] ; then
        echo "ERROR: Missing source directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$SRC_DIR/$TOOLCHAIN/llvm" ] ; then
        echo "ERROR: Source directory does not contain llvm sources: $SRC_DIR/$TOOLCHAIN/llvm"
        exit 1
    fi

    SRC_DIR=`cd $SRC_DIR; pwd`
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
    NDK_DIR=`cd $NDK_DIR; pwd`
    log "Using NDK directory: $NDK_DIR"

    # Check toolchain name
    #
    if [ -z "$TOOLCHAIN" ] ; then
        echo "ERROR: Missing toolchain name parameter. See --help for details."
        exit 1
    fi
}

set_parameters $PARAMETERS

prepare_target_build
prepare_abi_configure_build
set_toolchain_ndk $NDK_DIR $TOOLCHAIN

if [ "$MINGW" != "yes" -a "$DARWIN" != "yes" ] ; then
    dump "Using C compiler: $CC"
    dump "Using C++ compiler: $CXX"
fi

rm -rf $BUILD_OUT
mkdir -p $BUILD_OUT

MAKE_FLAGS=
if [ "$VERBOSE" = "yes" ]; then
    MAKE_FLAGS="VERBOSE=1"
fi

dump "Copy     : mclinker source"
MCLINKER_SRC_DIR=$BUILD_OUT/mclinker
mkdir -p $MCLINKER_SRC_DIR
fail_panic "Couldn't create mclinker source directory: $MCLINKER_SRC_DIR"

run copy_directory "$SRC_DIR/mclinker" "$MCLINKER_SRC_DIR"
fail_panic "Couldn't copy mclinker source: $MCLINKER_SRC_DIR"

cd $MCLINKER_SRC_DIR && run ./autogen.sh
fail_panic "Couldn't run autogen.sh in $MCLINKER_SRC_DIR"

# Remove aosp stuff away from PATH to prevent configure error.
aosp="${ANDROID_NDK_ROOT%/}"
aosp="${aosp%/ndk}"
aosp_regex="${aosp}/[^:]*:"
PATH="`echo $PATH | sed -e \"s#$aosp_regex##g\"`"
export PATH

for arch in $ARCHS; do
  dump "Rebuild for architecture $arch"

  # configure the toolchain
  dump "Configure: $TOOLCHAIN toolchain build"
  LLVM_BUILD_OUT=$BUILD_OUT/llvm/$arch
  mkdir -p $LLVM_BUILD_OUT && cd $LLVM_BUILD_OUT
  fail_panic "Couldn't cd into llvm build path: $LLVM_BUILD_OUT"

  toolchain_prefix=`get_default_toolchain_prefix_for_arch $arch`
  toolchain_name=`get_default_toolchain_name_for_arch $arch`
  if [ -n "$OPTION_GCC_VERSION" ]; then
    toolchain_name="${toolchain_name//${DEFAULT_GCC_VERSION}/${OPTION_GCC_VERSION}}"
  fi
  extra_configure_flags=''
  if [ $arch = "arm" ]; then
    extra_configure_flags='-mthumb -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16'
  fi

  run $BUILDTOOLS/make-standalone-toolchain.sh \
    --toolchain=$toolchain_name \
    --stl=stlport \
    --arch=$arch \
    --system=linux-$HOST_ARCH \
    --platform=android-9 \
    --install-dir=$BUILD_OUT/ndk-standalone-$arch
  fail_panic "Couldn't make standalone for $arch"

  mkdir -p $TOOLCHAIN_BUILD_PREFIX/$arch
  cp -f $BUILD_OUT/ndk-standalone-$arch/$toolchain_prefix/lib/libstlport_shared.so $TOOLCHAIN_BUILD_PREFIX/$arch

  CC=$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-gcc
  CXX=$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-g++
  export CC CXX

  run $SRC_DIR/$TOOLCHAIN/llvm/configure \
    --prefix=$TOOLCHAIN_BUILD_PREFIX/$arch \
    --host=$toolchain_prefix \
    --with-bug-report-url=$DEFAULT_ISSUE_TRACKER_URL \
    --enable-targets=$arch \
    --enable-optimized \
    --disable-polly \
    --with-clang-srcdir=/dev/null \
    --enable-shared \
    --with-extra-ld-options=-lstlport_shared \
    --disable-assertions \
    --with-extra-options="$extra_configure_flags"
  fail_panic "Couldn't configure llvm toolchain for $arch"

  dump "Building : llvm toolchain [this can take a long time]."
  unset CC CXX  # Need for BuildTools
  export CC CXX
  run make -j$NUM_JOBS $MAKE_FLAGS
  fail_panic "Couldn't compile llvm toolchain"

  # Copy binaries what we need
  mkdir -p $TOOLCHAIN_BUILD_PREFIX/$arch
  cp -f $LLVM_BUILD_OUT/Release/lib/libLLVM-${DEFAULT_LLVM_VERSION}.so $TOOLCHAIN_BUILD_PREFIX/$arch
  cp -f $LLVM_BUILD_OUT/Release/bin/le32-none-ndk-translate $TOOLCHAIN_BUILD_PREFIX/$arch
  cp -f $LLVM_BUILD_OUT/Release/bin/llc $TOOLCHAIN_BUILD_PREFIX/$arch

  # build mclinker only against default the LLVM version, once
  dump "Configure: mclinker against $TOOLCHAIN"
  MCLINKER_BUILD_OUT=$MCLINKER_SRC_DIR/build/$arch
  mkdir -p $MCLINKER_BUILD_OUT && cd $MCLINKER_BUILD_OUT
  fail_panic "Couldn't cd into mclinker build path: $MCLINKER_BUILD_OUT"

  CC=$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-gcc
  CXX="$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-g++ -lstlport_shared"
  export CC CXX

  run $MCLINKER_SRC_DIR/configure \
    --prefix=$TOOLCHAIN_BUILD_PREFIX/$arch \
    --with-llvm-config=$LLVM_BUILD_OUT/BuildTools/Release/bin/llvm-config \
    --with-llvm-shared-lib=$LLVM_BUILD_OUT/Release/lib/libLLVM-${DEFAULT_LLVM_VERSION}.so \
    --enable-targets=$arch \
    --host=$toolchain_prefix
  fail_panic "Couldn't configure mclinker for $arch"

  CXXFLAGS="$CXXFLAGS -fexceptions"  # optimized/ScriptParser.cc needs it
  export CXXFLAGS
  dump "Building : mclinker"
  cd $MCLINKER_BUILD_OUT
  run make -j$NUM_JOBS $MAKE_FLAGS CXXFLAGS="$CXXFLAGS"
  fail_panic "Couldn't compile mclinker"

  mkdir -p $TOOLCHAIN_BUILD_PREFIX/$arch
  cp -f $MCLINKER_BUILD_OUT/optimized/ld.mcld $TOOLCHAIN_BUILD_PREFIX/$arch

  # Strip
  STRIP=$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-strip
  find $TOOLCHAIN_BUILD_PREFIX/$arch -maxdepth 1 -type f -exec $STRIP --strip-all {} \;
done

TOOLCHAIN_BUILD_PREFIX="`cd $TOOLCHAIN_BUILD_PREFIX; pwd`"
dump "Done. Output is under $TOOLCHAIN_BUILD_PREFIX"
rm -rf $BUILD_OUT
