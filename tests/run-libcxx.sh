#!/bin/sh

DEVICE_arm="$1"
DEVICE_x86="$2"
DEVICE_mips="$3"

#STATIC="--static"

echo ================  llvm-libc++

  cd $NDK
  echo "### make standalone toolchain"
  TOOLCHAIN_DIR_ARM=${MYOPT}/android-ndk-api14-arm-4.8-libc++
  TOOLCHAIN_DIR_MIPS=${MYOPT}/android-ndk-api14-mips-4.8-libc++
  TOOLCHAIN_DIR_X86=${MYOPT}/android-ndk-api14-x86-4.8-libc++
  
  ./build/tools/make-standalone-toolchain.sh --platform=android-14 --llvm-version=3.3 \
        --install-dir=$TOOLCHAIN_DIR_ARM --toolchain=arm-linux-androideabi-4.8 --stl=libc++
  ./build/tools/make-standalone-toolchain.sh --platform=android-14 --llvm-version=3.3 \
        --install-dir=$TOOLCHAIN_DIR_MIPS --toolchain=mipsel-linux-android-4.8 --stl=libc++
  ./build/tools/make-standalone-toolchain.sh --platform=android-14 --llvm-version=3.3 \
        --install-dir=$TOOLCHAIN_DIR_X86 --toolchain=x86-4.8 --stl=libc++

  cd $NDK/sources/cxx-stl/llvm-libc++/libcxx/test

  if [ -z "$DEVICE_arm" ]; then
    echo "### clang3.3 armeabi-v7a: no device"
  else
    echo "### clang3.3 armeabi-v7a"
    ADB="adb -s $DEVICE_arm" PATH=$TOOLCHAIN_DIR_ARM/bin:$PATH \
      ./testit_android --abi=armeabi-v7a --cxx=arm-linux-androideabi-clang++ $STATIC
  fi
    
  if [ -z "$DEVICE_x86" ]; then
    echo "### clang3.3 x86: no device"
  else
    echo "### clang3.3 x86"
    ADB="adb -s $DEVICE_x86" PATH=$TOOLCHAIN_DIR_X86/bin:$PATH \
      ./testit_android --abi=x86 --cxx=i686-linux-android-clang++ $STATIC
  fi
    
  if [ -z "$DEVICE_mips" ]; then
    echo "### clang3.3 mips: no device"
  else
    echo "### clang3.3 mips"
    ADB="adb -s $DEVICE_mips" PATH=$TOOLCHAIN_DIR_MIPS/bin:$PATH \
      ./testit_android --abi=mips --cxx=mipsel-linux-android-clang++ $STATIC
  fi
