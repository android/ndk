#!/bin/sh
#
# Copyright (C) 2010 The Android Open Source Project
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
#  This shell script is used to copy the prebuilt GNU libstdc++ binaries
#  from a fresh toolchain directory generated with a command like
#  "build-gcc.sh --keep-libstdcxx".
#

. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<toolchain-dir> <ndk-dir>"
PROGRAM_DESCRIPTION="\
This program is used to extract fresh GNU libstdc++ binaries from a
toolchain directory that was generated with a command like:

  build-gcc.sh --keep-libstdcxx <toolchain-dir> <ndk-dir>

It will copy the files (headers and libraries) under <ndk-dir>/$GNUSTL_SUBDIR
unless you use the --out-dir option.
"

NDK_DIR="$ANDROID_NDK_ROOT"
register_var_option "--ndk-dir=<path>" NDK_DIR "Source NDK installation."

OUT_DIR=
register_var_option "--out-dir=<path>" OUT_DIR "Alternate installation location."

CLEAN_NDK=no
register_var_option "--clean-ndk" CLEAN_NDK "Remove binaries from NDK installation."

TOOLCHAIN=$DEFAULT_ARCH_TOOLCHAIN_NAME_arm
register_var_option "--toolchain=<name>" TOOLCHAIN "Specify toolchain name."

REVERSE=no
register_var_option "--reverse" REVERSE "Reverse copy direction."

extract_parameters "$@"

# Set HOST_TAG to linux-x86 on 64-bit Linux systems
force_32bit_binaries

set_parameters ()
{
    TOOLCHAIN_DIR="$1"
    NDK_DIR="$2"

    # Check source directory
    #
    if [ -z "$TOOLCHAIN_DIR" ] ; then
        echo "ERROR: Missing toolchain directory parameter. See --help for details."
        exit 1
    fi

    TOOLCHAIN_DIR2="$TOOLCHAIN_DIR/toolchains/$TOOLCHAIN/prebuilt/$HOST_TAG"
    if [ -d "$TOOLCHAIN_DIR2" ] ; then
        TOOLCHAIN_DIR="$TOOLCHAIN_DIR2"
        log "Auto-detecting toolchain installation: $TOOLCHAIN_DIR"
    fi

    if [ ! -d "$TOOLCHAIN_DIR/bin" -o ! -d "$TOOLCHAIN_DIR/lib" ] ; then
        echo "ERROR: Directory does not point to toolchain: $TOOLCHAIN_DIR"
        exit 1
    fi

    log "Using toolchain directory: $TOOLCHAIN_DIR"

    # Check NDK installation directory
    #
    if [ -z "$NDK_DIR" ] ; then
        echo "ERROR: Missing NDK directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: Not a valid directory: $NDK_DIR"
        exit 1
    fi

    log "Using NDK directory: $NDK_DIR"
}

set_parameters $PARAMETERS

parse_toolchain_name

# Determine output directory
if [ -n "$OUT_DIR" ] ; then
    if [ "$REVERSE" = "no" ] ; then
        mkdir -p "$OUT_DIR"
        fail_panic "Could not create directory: $OUT_DIR"
        log "Using specific output directory: $OUT_DIR"
    fi
else
    OUT_DIR="$NDK_DIR/$GNUSTL_SUBDIR"
    log "Using default output directory: $OUT_DIR"
    mkdir -p "$OUT_DIR"
fi

if [ ! -d "$OUT_DIR" ] ; then
    panic "Directory does not exist: $OUT_DIR"
fi

ABI_STL="$TOOLCHAIN_DIR/$ABI_CONFIGURE_TARGET"
ABI_STL_INCLUDE="$ABI_STL/include/c++/$GCC_VERSION"

OUT_INCLUDE="$OUT_DIR/include"
OUT_INCLUDE_ABI="$OUT_INCLUDE/$ABI_CONFIGURE_TARGET"
OUT_LIBS="$OUT_DIR/libs"
OUT_ABI="$OUT_DIR/$ABI_CONFIGURE_TARGET"

if [ "$REVERSE" = "no" ] ; then
    # Check the directories
    if [ ! -d "$ABI_STL" ] ; then
        panic "ERROR: Missing arch-specific directory: $ABI_STL"
    fi
    if [ ! -d "$ABI_STL_INCLUDE" ] ; then
        panic "ERROR: Missing toolchain-specific include directory: $ABI_STL_INCLUDE"
    fi

    # First, copy the headers
    copy_directory "$ABI_STL_INCLUDE" "$OUT_INCLUDE"

    # Copy platform-specific headers from $ABI_STL_INCLUDE/$ABI_CONFIGURE_TARGET
    # to relevant locations under $NDK_STL/libs/<arch>/include
    case "$ARCH" in
        arm)
            # We use the thumb version by default.
            copy_directory "$OUT_INCLUDE_ABI/thumb/bits" "$OUT_LIBS/armeabi/include/bits"
            copy_file_list "$ABI_STL/lib/thumb" "$OUT_LIBS/armeabi" "libstdc++.so libgnustl_shared.so"
            cp "$ABI_STL/lib/thumb/libstdc++.a" "$OUT_LIBS/armeabi-v7a/libgnustl_static.a"

            copy_directory "$OUT_INCLUDE_ABI/armv7-a/bits" "$OUT_LIBS/armeabi-v7a/include/bits"
            copy_file_list "$ABI_STL/lib/armv7-a" "$OUT_LIBS/armeabi-v7a" "libstdc++.so libgnustl_shared.so"
            cp "$ABI_STL/lib/armv7-a/libstdc++.a" "$OUT_LIBS/armeabi/libgnustl_static.a"
            ;;
        x86)
            copy_directory "$OUT_INCLUDE_ABI/bits" "$OUT_LIBS/x86/include/bits"
            copy_file_list "$ABI_STL/lib" "$OUT_LIBS/x86" "libstdc++.so"
            cp "$ABI_STL/lib/libstdc++.a" "$OUT_LIBS/x86/libgnustl_static.a"
            ;;
        *)
            dump "ERROR: Unsupported NDK architecture!"
    esac

    # Remove ABI-specific include headers
    run rm -rf "$OUT_INCLUDE_ABI"
    # We don't need the versioned libraries anymore (e.g. libstdc++.so.6
    # and libstdc++.so.6.0.13)
    run rm -f "$OUT_LIBS/"*/libstdc++.so.*
else # REVERSE = yes
    copy_directory "$OUT_DIR/include" "$ABI_STL_INCLUDE"
    ABI_STL_INCLUDE_TARGET="$ABI_STL_INCLUDE/$ABI_CONFIGURE_TARGET"
    mkdir -p "$ABI_STL_INCLUDE_TARGET"
    fail_panic "Can't create directory: $ABI_STL_INCLUDE_TARGET"
    case "$ARCH" in
        arm)
            copy_directory "$OUT_LIBS/armeabi/include/bits" "$ABI_STL_INCLUDE_TARGET/bits"
            copy_file_list "$OUT_LIBS/armeabi" "$ABI_STL/lib" "libstdc++.so"
            cp "$OUT_LIBS/armeabi/libgnustl_static.a" "$ABI_STL/lib/libstdc++.a"

            copy_directory "$OUT_LIBS/armeabi/include/bits" "$ABI_STL_INCLUDE_TARGET/thumb/bits"
            copy_file_list "$OUT_LIBS/armeabi" "$ABI_STL/lib/thumb" "libstdc++.so"
            cp "$OUT_LIBS/armeabi/libgnustl_static.a" "$ABI_STL/lib/thumb/libstdc++.a"

            copy_directory "$OUT_LIBS/armeabi-v7a/include/bits" "$ABI_STL_INCLUDE_TARGET/armv7-a/bits"
            copy_file_list "$OUT_LIBS/armeabi-v7a" "$ABI_STL/lib/armv7-a" "libstdc++.so"
            cp "$OUT_LIBS/armeabi-v7a/libgnustl_static.a" "$ABI_STL/lib/armv7-a/libstdc++.a"
            ;;
        x86)
            copy_directory "$OUT_LIBS/x86/include/bits" "$ABI_STL_INCLUDE_TARGET/bits"
            copy_file_list "$OUT_LIBS/x86" "$ABI_STL/lib" "libstdc++.so"
            cp "$OUT_LIBS/x86/libgnustl_static.a" "$ABI_STL/lib/libstdc++.a"
            ;;
        *)
            dump "ERROR: Unsupported NDK architecture!"
    esac
fi # REVERSE = yes
