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
# Rebuild all on-device toolchain for le32 target
#

PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "NDK installation directory"

BUILD_DIR=/tmp/ndk-$USER/build
register_var_option "--build-dir=<path>" BUILD_DIR "Specify temporary build dir."

OUT_DIR=/tmp/ndk-$USER/out
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory directly."

ABI=armeabi-v7a
register_var_option "--abi=<target>" ABI "List which emulator target you use"

NO_SYNC=
do_no_sync_option () { NO_SYNC=yes; }
register_option "--no-sync" do_no_sync_option "Do not push sysroot to device /data/local/tmp/"

TESTING=
do_testing_option () { TESTING=yes; }
register_option "--testing" do_testing_option "Copy each c++ libraries for (even for GPL stuff)"

SHARED=
do_shared_option () { SHARED=yes; }
register_option "--shared" do_shared_option "Build libLLVM*.so shared by on-device llvm toolchain, and link lib*stl_shared.so"

register_jobs_option

PROGRAM_PARAMETERS="<toolchain-src-dir>"
PROGRAM_DESCRIPTION=\
"This script can be used to copy all components that on-device compilation needs.
Also, it will compile on-device LLVM-related toolchain."

extract_parameters "$@"

# Check toolchain source path
SRC_DIR="$PARAMETERS"
check_toolchain_src_dir "$SRC_DIR"
BUILDTOOLS=$ANDROID_NDK_ROOT/build/tools
run rm -rf $BUILD_DIR
run mkdir -p $BUILD_DIR $OUT_DIR

FLAGS=
if [ "$VERBOSE" = "yes" ]; then
  FLAGS=$FLAGS" --verbose"
fi
if [ "$VERBOSE2" = "yes" ]; then
  FLAGS=$FLAGS" --verbose"
fi
FLAGS="$FLAGS -j$NUM_JOBS"
if [ "$SHARED" = "yes" ]; then
  FLAGS="$FLAGS --shared"
fi

TMP_OUT_DIR=/tmp/ndk-$USER/on_device_out
FLAGS="$FLAGS --out-dir=$TMP_OUT_DIR"
ARCH="$(convert_abi_to_arch $ABI)"
GCC_TOOLCHAIN_VERSION=`cat $NDK_DIR/toolchains/llvm-$DEFAULT_LLVM_VERSION/setup.mk | grep '^TOOLCHAIN_VERSION' | awk '{print $3'}`

SYSROOT=$NDK_DIR/$(get_default_platform_sysroot_for_arch $ARCH)
OUT_SYSROOT=$OUT_DIR

run mkdir -p $OUT_SYSROOT/usr/bin
run mkdir -p $OUT_SYSROOT/usr/lib

dump "Copy platform CRT files..."
run cp -r $SYSROOT/usr/lib/crtbegin_dynamic.o $OUT_SYSROOT/usr/lib
run cp -r $SYSROOT/usr/lib/crtbegin_so.o $OUT_SYSROOT/usr/lib
run cp -r $SYSROOT/usr/lib/crtend_android.o $OUT_SYSROOT/usr/lib
run cp -r $SYSROOT/usr/lib/crtend_so.o $OUT_SYSROOT/usr/lib

dump "Copy $ABI gabi++ library"
run cp -f $NDK_DIR/$GABIXX_SUBDIR/libs/$ABI/libgabi++_shared.so $OUT_SYSROOT/usr/lib

dump "Copy $ABI libportable library"
run cp -f $NDK_DIR/$LIBPORTABLE_SUBDIR/libs/$ABI/libportable.a $OUT_SYSROOT/usr/lib
run cp -f $NDK_DIR/$LIBPORTABLE_SUBDIR/libs/$ABI/libportable.wrap $OUT_SYSROOT/usr/lib

dump "Copy $ABI compiler-rt library"
run cp -f $NDK_DIR/$COMPILER_RT_SUBDIR/libs/$ABI/libcompiler_rt_static.a $OUT_SYSROOT/usr/lib

dump "Copy $ABI gccunwind library"
run cp -f $NDK_DIR/$GCCUNWIND_SUBDIR/libs/$ABI/libgccunwind.a $OUT_SYSROOT/usr/lib

if [ "$TESTING" = "yes" ]; then
  dump "Copy stuff for testing"
  run cp -f $NDK_DIR/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI/libsupc++.a $OUT_SYSROOT/usr/lib
  run cp -f $NDK_DIR/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI/libgnustl_static.a $OUT_SYSROOT/usr/lib
  run cp -f $NDK_DIR/$GNUSTL_SUBDIR/$GCC_TOOLCHAIN_VERSION/libs/$ABI/libgnustl_shared.so $OUT_SYSROOT/usr/lib

  run cp -f $NDK_DIR/$STLPORT_SUBDIR/libs/$ABI/libstlport_static.a $OUT_SYSROOT/usr/lib
  run cp -f $NDK_DIR/$STLPORT_SUBDIR/libs/$ABI/libstlport_shared.so $OUT_SYSROOT/usr/lib

  run cp -f $NDK_DIR/$GABIXX_SUBDIR/libs/$ABI/libgabi++_static.a $OUT_SYSROOT/usr/lib
  run cp -f $NDK_DIR/$GABIXX_SUBDIR/libs/$ABI/libgabi++_shared.so $OUT_SYSROOT/usr/lib
fi

dump "Strip $ABI binaries"
STRIP=$NDK_DIR/$(get_default_toolchain_binprefix_for_arch $ARCH)strip
run find $OUT_SYSROOT/usr/lib \( -name "*\.so" \) -exec $STRIP --strip-all {} \;

dump "Build $ABI LLVM toolchain from $SRC_DIR ..."
run $BUILDTOOLS/build-device-llvm.sh $FLAGS --abis=$ABI --gcc-version=$GCC_TOOLCHAIN_VERSION $SRC_DIR $NDK_DIR
fail_panic "Could not build le32 LLVM toolchain!"
run mv -f $TMP_OUT_DIR/$ABI/SOURCES $OUT_SYSROOT/usr
if [ "$SHARED" = "yes" ]; then
  run mv -f $TMP_OUT_DIR/$ABI/lib*.so $OUT_SYSROOT/usr/lib
fi
run mv -f $TMP_OUT_DIR/$ABI/* $OUT_SYSROOT/usr/bin
run rmdir $TMP_OUT_DIR/$ABI
run rmdir $TMP_OUT_DIR

if [ "$NO_SYNC" != "yes" ]; then
  dump "Push on-device $ABI toolchain sysroot to /data/local/tmp/"
  run adb shell rm -rf /data/local/tmp/*
  run adb push $OUT_SYSROOT /data/local/tmp/
  fail_panic "Could not push sysroot!"
fi

dump "Done."
exit 0
