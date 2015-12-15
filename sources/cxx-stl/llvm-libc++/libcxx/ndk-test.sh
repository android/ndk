#!/bin/bash

if [ -z "$NDK" ]; then
    >&2 echo "Error: $$NDK must be set in your environment."
    exit 1
fi

ABI=$1
if [ -z "$ABI" ]; then
    >&2 echo "usage: $(basename $0) ABI"
    exit 1
fi

case "$ABI" in
    armeabi*)
        ARCH=arm
        TRIPLE=arm-linux-androideabi
        ;;
    arm64-v8a)
        ARCH=arm64
        TRIPLE=aarch64-linux-android
        ;;
    mips)
        ARCH=mips
        TRIPLE=mipsel-linux-android
        ;;
    mips64)
        ARCH=mips64
        TRIPLE=mips64el-linux-android
        ;;
    x86)
        ARCH=x86
        TRIPLE=i686-linux-android
        ;;
    x86_64)
        ARCH=x86_64
        TRIPLE=x86_64-linux-android
        ;;
    *)
        >&2 echo "Unknown ABI: $ABI"
        exit 1
        ;;
esac

HOST_TAG=linux-x86_64

LIT_ARGS=${@:2}

LIBCXX_DIR=$NDK/sources/cxx-stl/llvm-libc++/libcxx
sed -e "s:%ABI%:$ABI:g" -e "s:%TRIPLE%:$TRIPLE:g" \
    -e "s:%ARCH%:$ARCH:g" \
    $LIBCXX_DIR/test/lit.ndk.cfg.in > $LIBCXX_DIR/test/lit.site.cfg

adb push $LIBCXX_DIR/../libs/$ABI/libc++_shared.so /data/local/tmp
lit -sv $LIT_ARGS $LIBCXX_DIR/test
