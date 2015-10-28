#!/bin/bash

if [ -z "$NDK" ]; then
    >&2 echo "Error: $$NDK must be set in your environment."
    exit 1
fi

adb push $NDK/sources/cxx-stl/llvm-libc++/libs/arm64-v8a/libc++_shared.so /data/local/tmp
lit -sv $* $NDK/sources/cxx-stl/llvm-libc++/libcxx/test
