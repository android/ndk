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
ABIS="$PREBUILT_ABIS"
BUILDTOOLS=$ANDROID_NDK_ROOT/build/tools

BUILD_OUT=/tmp/ndk-$USER/build/toolchain
OPTION_BUILD_OUT=
register_var_option "--build-out=<path>" OPTION_BUILD_OUT "Set temporary build directory"

OUT_DIR="$BUILD_OUT/../prefix"
OPTION_OUT_DIR=
register_var_option "--out-dir=<path>" OPTION_OUT_DIR "On-device toolchain will be put at <path>"

OPTION_ABIS=
register_var_option "--abis=<armeabi,armeabi-v7a,x86,mips,armeabi-v7a-hard>" OPTION_ABIS "Default: $ABIS"

OPTION_GCC_VERSION=
register_var_option "--gcc-version=<version>" OPTION_GCC_VERSION "Specify GCC toolchain version [Default: $DEFAULT_GCC_VERSION]"

STL=stlport
register_var_option "--stl=<name>" STL "Specify C++ STL"

SHARED=
do_shared_option () { SHARED=yes; }
register_option "--shared" do_shared_option "Build libLLVM*.so shared by on-device llvm toolchain, and link lib*stl_shared.so"

register_jobs_option

extract_parameters "$@"

prepare_canadian_toolchain /tmp/ndk-$USER/build

fix_option ABIS "$OPTION_ABIS" "on-device architecture"
ABIS=($commas_to_spaces $ABIS)
fix_option BUILD_OUT "$OPTION_BUILD_OUT" "build directory"
setup_default_log_file $BUILD_OUT/config.log
fix_option OUT_DIR "$OPTION_OUT_DIR" "On-device toolchain will be put at <path>"
TOOLCHAIN_BUILD_PREFIX="$OUT_DIR"

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

for abi in $ABIS; do
  dump "Rebuild for ABI $abi with $STL"

  # configure the toolchain
  dump "Configure: $TOOLCHAIN toolchain build"
  LLVM_BUILD_OUT=$BUILD_OUT/llvm/$abi
  run mkdir -p $LLVM_BUILD_OUT && cd $LLVM_BUILD_OUT
  fail_panic "Couldn't cd into llvm build path: $LLVM_BUILD_OUT"

  arch="$(convert_abi_to_arch $abi)"
  if [ -n "$OPTION_GCC_VERSION" ]; then
    GCCVER=$OPTION_GCC_VERSION
  else
    GCCVER=$(get_default_gcc_version_for_arch $arch)
  fi
  toolchain_prefix=`get_default_toolchain_prefix_for_arch $arch`
  toolchain_name=`get_toolchain_name_for_arch $arch $GCCVER`
  CFLAGS="-fomit-frame-pointer -fstrict-aliasing -ffunction-sections -fdata-sections"
  case $abi in
    armeabi)
      # No -mthumb, because ./llvm/lib/Target/ARM/ARMJITInfo.cpp now contain inline assembly
      # code such as "stmdb sp!,{r0,r1,r2,r3,lr}" which doesn't support thumb1
      CFLAGS=$CFLAGS" -march=armv5te -msoft-float"
      ;;
    armeabi-v7a|armeabi-v7a-hard)
      CFLAGS=$CFLAGS" -mthumb -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16"
      if [ "$arbi" = "armeabi-v7a-hard" ]; then
        CFLAGS=$CFLAGS" -mhard-float -D_NDK_MATH_NO_SOFTFP=1 -Wl,--no-warn-mismatch -lm_hard"
      fi
      ;;
    mips)
      CFLAGS=$CFLAGS" -fmessage-length=0 -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers"
      ;;
  esac

  run $BUILDTOOLS/make-standalone-toolchain.sh \
    --toolchain=$toolchain_name \
    --stl=$STL \
    --arch=$arch \
    --system=$HOST_TAG \
    --platform=android-19 \
    --install-dir=$BUILD_OUT/ndk-standalone-$arch
  fail_panic "Couldn't make standalone for $arch"

  run mkdir -p $TOOLCHAIN_BUILD_PREFIX/$abi
  if [ "$SHARED" = "yes" ]; then
    run cp -f $BUILD_OUT/ndk-standalone-$arch/$toolchain_prefix/lib/lib${STL}_shared.so $TOOLCHAIN_BUILD_PREFIX/$abi
  fi

  CC=$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-gcc
  CXX=$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-g++
  export CC CXX

  EXTRA_LLVM_CONFIG=""
  EXTRA_MCLINKER_CONFIG=""
  if [ "$SHARED" = "yes" ]; then
    EXTRA_LLVM_CONFIG="--enable-shared --with-extra-ld-options=-l${STL}_shared"
    EXTRA_MCLINKER_CONFIG="--with-llvm-shared-lib=$LLVM_BUILD_OUT/Release/lib/libLLVM-${DEFAULT_LLVM_VERSION}.so"
  fi

  run $SRC_DIR/$TOOLCHAIN/llvm/configure \
    --prefix=$TOOLCHAIN_BUILD_PREFIX/$abi \
    --host=$toolchain_prefix \
    --with-bug-report-url=$DEFAULT_ISSUE_TRACKER_URL \
    --enable-targets=$arch \
    --enable-optimized \
    --enable-shrink-binary-size \
    --disable-polly \
    --with-clang-srcdir=/dev/null \
    --disable-assertions \
    --disable-terminfo \
    --with-extra-options="$CFLAGS" \
    $EXTRA_LLVM_CONFIG
  fail_panic "Couldn't configure llvm toolchain for ABI $abi"

  dump "Building : llvm toolchain [this can take a long time]."
  unset CC CXX  # Need for BuildTools
  export CC CXX
  run make -j$NUM_JOBS $MAKE_FLAGS
  fail_panic "Couldn't compile llvm toolchain"

  # Copy binaries what we need
  run mkdir -p $TOOLCHAIN_BUILD_PREFIX/$abi
  if [ "$SHARED" = "yes" ]; then
    run cp -f $LLVM_BUILD_OUT/Release/lib/libLLVM-${DEFAULT_LLVM_VERSION}.so $TOOLCHAIN_BUILD_PREFIX/$abi
  fi
  run cp -f $LLVM_BUILD_OUT/Release/bin/ndk-translate $TOOLCHAIN_BUILD_PREFIX/$abi
  run cp -f $LLVM_BUILD_OUT/Release/bin/llc $TOOLCHAIN_BUILD_PREFIX/$abi
  run cd $TOOLCHAIN_BUILD_PREFIX/$abi && ln -s ndk-translate le32-none-ndk-translate
  run cd $TOOLCHAIN_BUILD_PREFIX/$abi && ln -s ndk-translate le64-none-ndk-translate

  # build mclinker only against default the LLVM version, once
  dump "Configure: mclinker against $TOOLCHAIN"
  MCLINKER_BUILD_OUT=$MCLINKER_SRC_DIR/build/$abi
  run mkdir -p $MCLINKER_BUILD_OUT && cd $MCLINKER_BUILD_OUT
  fail_panic "Couldn't cd into mclinker build path: $MCLINKER_BUILD_OUT"

  CC="$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-gcc $CFLAGS"
  CXX="$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-g++ $CFLAGS"
  if [ "$SHARED" = "yes" ]; then
    CXX="$CXX -l${STL}_shared"
  fi
  export CC CXX

  run $MCLINKER_SRC_DIR/configure \
    --prefix=$TOOLCHAIN_BUILD_PREFIX/$abi \
    --with-llvm-config=$LLVM_BUILD_OUT/BuildTools/Release/bin/llvm-config \
    --enable-targets=$arch \
    --host=$toolchain_prefix \
    --enable-shrink-binary-size \
    $EXTRA_MCLINKER_CONFIG
  fail_panic "Couldn't configure mclinker for ABI $abi"

  CXXFLAGS="$CXXFLAGS -fexceptions"  # optimized/ScriptParser.cc needs it
  export CXXFLAGS
  dump "Building : mclinker"
  cd $MCLINKER_BUILD_OUT
  run make -j$NUM_JOBS $MAKE_FLAGS CXXFLAGS="$CXXFLAGS"
  fail_panic "Couldn't compile mclinker"

  run mkdir -p $TOOLCHAIN_BUILD_PREFIX/$abi
  if [ -f $MCLINKER_BUILD_OUT/tools/lite/ld.lite ]; then
    run cp -f $MCLINKER_BUILD_OUT/tools/lite/ld.lite $TOOLCHAIN_BUILD_PREFIX/$abi/ld.mcld
  else
    run cp -f $MCLINKER_BUILD_OUT/optimized/ld.mcld $TOOLCHAIN_BUILD_PREFIX/$abi
  fi
  fail_panic "Couldn't copy mclinker"

  # Strip
  STRIP=$BUILD_OUT/ndk-standalone-$arch/bin/$toolchain_prefix-strip
  find $TOOLCHAIN_BUILD_PREFIX/$abi -maxdepth 1 -type f -exec $STRIP --strip-all {} \;

  # copy SOURCES
  run cp "$SRC_DIR/SOURCES" $TOOLCHAIN_BUILD_PREFIX/$abi
done

TOOLCHAIN_BUILD_PREFIX="`cd $TOOLCHAIN_BUILD_PREFIX; pwd`"

dump "Done. Output is under $TOOLCHAIN_BUILD_PREFIX"
if [ -z "$OPTION_BUILD_OUT" ] ; then
  rm -rf $BUILD_OUT
fi
