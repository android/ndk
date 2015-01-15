#!/bin/bash
set -e -x

NDK_TOP=$ANDROID_BUILD_TOP/ndk
GCC_TOOLCHAIN=$ANDROID_BUILD_TOP/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9
CLANG=$ANDROID_BUILD_TOP/prebuilts/clang/linux-x86/host/3.6/bin/clang
CXX_STL=$NDK_TOP/sources/cxx-stl
SYSROOT=$NDK_TOP/platforms/android-21/arch-arm/
TRIPLE=armv7a-linux-androideabi
CXX_FLAGS='-fvisibility=hidden -fvisibility-inlines-hidden'
OUT=$NDK_TOP/out/libcxx
LIT_ARGS='-sv'

cd $ANDROID_BUILD_TOP/ndk

rm -rf $OUT
mkdir -p $OUT
cd $OUT
cmake \
  -DCMAKE_C_COMPILER=$CLANG \
  -DCMAKE_CXX_COMPILER=$CLANG++ \
  -DCMAKE_CXX_FLAGS="$CXX_FLAGS" \
  -DLIBCXX_CROSSCOMPILING=ON \
  -DLIBCXX_CXX_ABI=libcxxabi \
  -DLIBCXX_CXX_ABI_LIBRARY_PATH=$OUT/../libcxxabi/lib \
  -DLIBCXX_ENABLE_SHARED=OFF \
  -DLIBCXX_GCC_TOOLCHAIN="$GCC_TOOLCHAIN" \
  -DLIBCXX_LIBCXXABI_INCLUDE_PATHS=$CXX_STL/libcxxabi/include/ \
  -DLIBCXX_LIT_VARIANT=libcxx.ndk \
  -DLIBCXX_SYSROOT="$SYSROOT" \
  -DLIBCXX_TARGET_TRIPLE="$TRIPLE" \
  -DLIT_EXECUTABLE=$ANDROID_BUILD_TOP/external/llvm/utils/lit/lit.py \
  -DLLVM_LIT_ARGS="$LIT_ARGS" \
  $CXX_STL/libcxx \

make -j40 check-libcxx
