#!/bin/bash
set -e -x

NDK_TOP=$ANDROID_BUILD_TOP/ndk
GCC_TOOLCHAIN=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9
CLANG=$ANDROID_BUILD_TOP/prebuilts/clang/linux-x86/host/3.6/bin/clang
CXX_STL=$NDK_TOP/sources/cxx-stl
SYSROOT=$NDK_TOP/platforms/android-21/arch-arm/
TRIPLE=armv7-linux-androideabi
CXX_FLAGS='-fvisibility=hidden -fvisibility-inlines-hidden'
OUT=$NDK_TOP/out/libcxxabi

cd $ANDROID_BUILD_TOP/ndk

rm -rf $OUT
mkdir -p $OUT
cd $OUT
cmake \
  -DCMAKE_C_COMPILER=$CLANG \
  -DCMAKE_CXX_COMPILER=$CLANG++ \
  -DCMAKE_CXX_FLAGS="$CXX_FLAGS" \
  -DCMAKE_VERBOSE_MAKEFILE=ON \
  -DLIBCXXABI_CROSSCOMPILING=ON \
  -DLIBCXXABI_ENABLE_SHARED=OFF \
  -DLIBCXXABI_GCC_TOOLCHAIN="$GCC_TOOLCHAIN" \
  -DLIBCXXABI_LIBCXX_INCLUDES=$CXX_STL/libcxx/include \
  -DLIBCXXABI_SYSROOT="$SYSROOT" \
  -DLIBCXXABI_TARGET_TRIPLE="$TRIPLE" \
  -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
  $CXX_STL/libcxxabi \

make -j40
