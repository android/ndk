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

. `dirname $0`/prebuilt-common.sh
PROGDIR=`dirname $0`
PROGNAME=`basename $0`

# This sets HOST_TAG to linux-x86 or darwin-x86 on 64-bit systems
force_32bit_binaries

PLATFORM=
register_option "--platform=<name>" do_platform "Specify API level [autodetect]"
do_platform () { PLATFORM=$1; }

ARCH=
register_option "--arch=<name>" do_arch "Specify architecture name [autodetect]"
do_arch () { ARCH=$1; }

OUT_DIR=/tmp/ndk-$USER/platforms-import
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory"

TOOLCHAIN_PREFIX=

PROGRAM_PARAMETERS="<build-path>"
PROGRAM_DESCRIPTION=\
"This script is used to import the NDK-exposed system libraries and headers from
an existing platform build, and copy them to a temporary directory that will later
be processed by another script (dev-platform-pack.sh) to archive them properly into
the NDK directory structure (e.g. under development/ndk/platforms/).

By default, the files will be copied to $OUT_DIR, but you can
override this with the --out-dir=<path> option.

The script also extracts the list of dynamic symbols exported by system libraries,
filtering those that should not be exposed through the NDK. You can disable this
with the --no-symbol-filtering option.

The <build-path> parameter must point to the platform build output directory
of a valid Android reference platform build. This is the value of the
\$ANDROID_PRODUCT_OUT variable when doing a build configured with the 'lunch'
utility. It usually looks like <top>/out/target/product/<name> where <top> is
the top-level Android source tree, and <name> is the build product name
(e.g. 'generic' or 'generic_x86' for emulator-specific builds).

The API level is auto-detected from the content of <build-path>, but you
can override it with --platform=<number>.

This script is really in charge of the following tasks:
  
  1/ Detect the platform build's API level by parsing the build.prop
     file. This can overriden with --platform=<number>

  2/ Detect the platform build's target CPU architecture by parsing
     the build.prop file. This can be overriden with --arch=<name>

  3/ Copy system headers from \$ANDROID/framework/base/ and other
     locations into \$OUT_DIR/android-\$API/include or
     \$OUT_DIR/android-\$API/arch-\$ARCH/include

  4/ Locate system shared libraries from \$ANDROID_PRODUCT_OUT/system/lib
     and generate symbol files in \$OUT_DIR/android-\$API/arch-\$ARCH/symbols.

  5/ Copy a few system static libraries (libc.a, libm.a, etc...) used
     to generate static executables. As well as a few key object files
     required by the C runtime (e.g. crtbegin_dynamic.o), when needed.

"


extract_parameters "$@"

if [ -z "$PARAMETERS" ] ; then
    if [ -z "$ANDROID_PRODUCT_OUT" ]; then
        dump "ERROR: Missing path parameter to platform build, see --help for details"
        exit 1
    fi
    # ANDROID_PRODUCT_OUT is defined, so use it
    log "Auto-config: using build path: $ANDROID_PRODUCT_OUT"
else
    ANDROID_PRODUCT_OUT=$PARAMETERS
fi

# Sanity checks for the Android build path
if [ ! -d "$ANDROID_PRODUCT_OUT" ]; then
    dump "ERROR: Not a directory: $ANDROID_PRODUCT_OUT"
    dump "Please point to a valid and complete Android build directory"
    exit 1
fi

BUILD_PROP=$ANDROID_PRODUCT_OUT/system/build.prop
if [ ! -f "$BUILD_PROP" ]; then
    dump "ERROR: Missing file: $BUILD_PROP"
    dump "Please point to a valid and complete Android build directory"
    exit 1
fi
log "Extracting build properties from: $BUILD_PROP"

# Try to get the architecture from build.prop if needed
if [ -z "$ARCH" ]; then
    CPU_ABI=$(awk -F '=' '$1 == "ro.product.cpu.abi"  { print $2; }' $BUILD_PROP)
    if [ $? != 0 ]; then
        dump "ERROR: Could not extract CPU ABI from $BUILD_PROP"
        dump "Please use --arch=<name> to override this. See --help"
        exit 1
    fi
    log "Found target CPU ABI: $CPU_ABI"
    ARCH=$(convert_abi_to_arch $CPU_ABI)
    if [ -z "$ARCH" ]; then
        dump "ERROR: Can't translate $CPU_ABI ABI into CPU architecture!"
        exit 1
    fi
    log "Auto-config: --arch=$ARCH"
fi

# Try to get the API level from build.prop if needed
if [ -z "$PLATFORM" ]; then
    PLATFORM=$(awk -F '=' '$1 == "ro.build.version.sdk" { print $2; }' $BUILD_PROP)
    if [ $? != 0 ] ; then
        dump "WARNING: Could not extract default platform level from $BUILD_PROP!"
        dump "Please use --platform=<name> to override this. See --help"
        exit 1
    fi
    if [ -z "$PLATFORM" ]; then
        dump "ERROR: Couldn't extract API level from: $BUILD_PROP"
        exit 1
    fi
    log "Auto-config: --platform=$PLATFORM"
fi

# Normalize platform name, i.e.
#  3         -> 3
#  android-3 -> 3
#
PLATFORM=${PLATFORM##android-}

PLATFORM_ROOT=$OUT_DIR/android-$PLATFORM/arch-$ARCH
log "Using platform destination path: $PLATFORM_ROOT"

# Return the list of files under a given directory
# $1: directory
# $2: (optional) file patterns
list_regular_files_in ()
{
    local DIR="$1"
    shift
    local PATTERNS="$@"
    if [ -z "$PATTERNS" ]; then
        PATTERNS="."
    fi
    cd "$DIR" && find $PATTERNS -type f | sed -e 's!^./!!g'
}

# Check that a given platform level was listed on the command line
# $1: Platform numerical level (e.g. '3')
# returns true if the platform is listed
platform_check ()
{
    [ "$PLATFORM" -ge "$1" ]
}

# Determine Android build tree root
ANDROID_ROOT=`cd $ANDROID_PRODUCT_OUT/../../../.. && pwd`
log "Android build tree root: $ANDROID_ROOT"
log "Android product out: $ANDROID_PRODUCT_OUT"

if [ -z "$TOOLCHAIN_PREFIX" ]; then
    TOOLCHAIN_NAME=$(get_default_toolchain_name_for_arch $ARCH)
    TOOLCHAIN_PREFIX=$(get_default_toolchain_prefix_for_arch $ARCH)
    TOOLCHAIN_PREFIX=$(get_toolchain_install $ANDROID_NDK_ROOT $TOOLCHAIN_NAME)/bin/$TOOLCHAIN_PREFIX
    TOOLCHAIN_PREFIX=${TOOLCHAIN_PREFIX%%-}
    if [ -z "$TOOLCHAIN_PREFIX" ]; then
        echo "ERROR: Unsupported architecture"
        exit 1
    fi
    log "Auto-config: --toolchain-prefix=$TOOLCHAIN_PREFIX"
fi

if [ ! -d "$(dirname $TOOLCHAIN_PREFIX)" ]; then
    echo "ERROR: Toolchain not installed, missing directory: $(dirname $TOOLCHAIN_PREFIX)"
    exit 1
fi

if [ ! -f "$TOOLCHAIN_PREFIX-readelf" ]; then
    echo "ERROR: Toolchain not installed, missing program: $TOOLCHAIN_PREFIX-readelf"
    exit 1
fi


# 'Copy' a given system library. This really reads the shared library to
# to generate a small shell version that will be installed at the destination
# $1: Library name (e.g. libEGL.so)
#
copy_system_shared_library ()
{
    local src="$ANDROID_PRODUCT_OUT/system/lib/$1.so"
    if [ ! -f "$src" ] ; then
        dump "ERROR: Missing system library: $src"
        exit 1
    fi
    local dst="$PLATFORM_ROOT/lib/$1.so"
    mkdir -p `dirname "$dst"` && cp "$src" "$dst"
}

copy_system_static_library ()
{
    local src="$ANDROID_PRODUCT_OUT/obj/STATIC_LIBRARIES/$1_intermediates/$1.a"
    if [ ! -f "$src" ] ; then
        dump "ERROR: Missing system static library: $src"
        exit 1
    fi
    local dst="$PLATFORM_ROOT/lib/$1.a"
    log "Copying system static library: $1.a"
    mkdir -p `dirname "$dst"` && cp -f "$src" "$dst"
}

copy_system_object_file ()
{
    local src="$ANDROID_PRODUCT_OUT/obj/lib/$1.o"
    if [ ! -f "$src" ] ; then
        dump "ERROR: Missing system object file: $src"
        exit 1
    fi
    local dst="$PLATFORM_ROOT/lib/$1.o"
    log "Copying system object file: $1.o"
    mkdir -p `dirname "$dst"` &&
    cp -f "$src" "$dst"
}

# Copy the content of a given directory to $SYSROOT/usr/include
# $1: Source directory
# $2+: List of headers
copy_system_headers ()
{
    local srcdir="$1"
    shift
    local header
    log "Copying system headers from: $srcdir"
    for header; do
        log "  $header"
        local src="$srcdir/$header"
        local dst="$PLATFORM_ROOT/../include/$header"
        if [ ! -f "$srcdir/$header" ] ; then
            dump "ERROR: Missing system header: $srcdir/$header"
            exit 1
        fi
        mkdir -p `dirname "$dst"` && cp -f "$src" "$dst"
        if [ $? != 0 ] ; then
            dump "ERROR: Could not copy system header: $src"
            dump "Target location: $dst"
            exit 1
        fi
    done
}

# Copy all headers found under $1
# $1: source directory
copy_system_headers_from ()
{
    local headers=$(list_regular_files_in "$1")
    copy_system_headers $1 $headers
}

# Same as copy_system_headers, but for arch-specific files
# $1: Source directory
# $2+: List of headers
copy_arch_system_headers ()
{
    local srcdir="$1"
    shift
    local header
    for header; do
        log "Copying $arch system header: $header"
        local src="$srcdir/$header"
        local dst="$PLATFORM_ROOT/include/$header"
        if [ ! -f "$srcdir/$header" ] ; then
            dump "ERROR: Missing $ARCH system header: $srcdir/$header"
            exit 1
        fi
        mkdir -p `dirname "$dst"` && cp -f "$src" "$dst"
        if [ $? != 0 ] ; then
            dump "ERROR: Could not copy $ARCH system header: $src"
            dump "Target location: $dst"
            exit 1
        fi
    done
}

copy_arch_system_headers_from ()
{
    local headers=$(list_regular_files_in "$1")
    copy_arch_system_headers $1 $headers
}

copy_arch_kernel_headers_from ()
{
    local headers=$(list_regular_files_in "$1" asm)
    copy_arch_system_headers $1 $headers
}

# Probe for the location of the OpenSLES sources in the
# platform tree. This used to be system/media/opensles but
# was moved to system/media/wilhelm in Android 4.0
OPENSLES_SUBDIR=system/media/wilhelm
if [ ! -d "$ANDROID_ROOT/$OPENSLES_SUBDIR" ]; then
    OPENSLES_SUBDIR=system/media/opensles
fi

# Now do the work

# API level 3
if platform_check 3; then
    copy_system_shared_library libc
    copy_system_static_library libc
    copy_system_headers_from $ANDROID_ROOT/bionic/libc/include
    copy_arch_system_headers_from $ANDROID_ROOT/bionic/libc/arch-$ARCH/include
    copy_arch_kernel_headers_from $ANDROID_ROOT/bionic/libc/kernel/arch-$ARCH

    copy_system_object_file crtbegin_dynamic
    copy_system_object_file crtbegin_static
    copy_system_object_file crtend_android
    case $ARCH in
    x86)
        copy_system_object_file crtbegin_so
        copy_system_object_file crtend_so
        ;;
    esac

    copy_system_shared_library libm
    copy_system_static_library libm
    copy_system_headers $ANDROID_ROOT/bionic/libm/include math.h
    case "$ARCH" in
    x86 )
        copy_arch_system_headers $ANDROID_ROOT/bionic/libm/include/i387 fenv.h
        ;;
    * )
        copy_arch_system_headers $ANDROID_ROOT/bionic/libm/$ARCH fenv.h
        ;;
    esac

    # The <dlfcn.h> header was already copied from bionic/libc/include
    copy_system_shared_library libdl
    # There is no libdl.a at the moment, we might need one in
    # the future to build gdb-7.1.x though.

    copy_system_shared_library libz
    copy_system_static_library libz
    copy_system_headers $ANDROID_ROOT/external/zlib zconf.h zlib.h

    copy_system_shared_library liblog
    copy_system_headers $ANDROID_ROOT/system/core/include android/log.h

    # NOTE: We do not copy the C++ headers, they are part of the NDK
    #        under $NDK/source/cxx-stl. They were separated from the rest
    #        of the platform headers in order to make room for other STL
    #        implementations (e.g. STLport or GNU Libstdc++-v3)
    #
    copy_system_shared_library libstdc++
    copy_system_static_library libstdc++

    copy_system_headers $ANDROID_ROOT/libnativehelper/include/nativehelper jni.h
fi

# API level 4
if platform_check 4; then
    copy_system_shared_library libGLESv1_CM
    copy_system_headers $ANDROID_ROOT/frameworks/base/opengl/include \
        GLES/gl.h \
        GLES/glext.h \
        GLES/glplatform.h

    copy_system_headers $ANDROID_ROOT/frameworks/base/opengl/include \
        KHR/khrplatform.h
fi

# API level 5
if platform_check 5; then
    copy_system_shared_library libGLESv2
    copy_system_headers $ANDROID_ROOT/frameworks/base/opengl/include \
        GLES2/gl2.h \
        GLES2/gl2ext.h \
        GLES2/gl2platform.h
fi

# API level 8
if platform_check 8; then
    copy_system_shared_library libandroid
    copy_system_shared_library libjnigraphics
    copy_system_headers $ANDROID_ROOT/frameworks/base/native/include \
        android/bitmap.h
fi

# API level 9
if platform_check 9; then
    case $ARCH in
    arm)
        copy_system_object_file crtbegin_so
        copy_system_object_file crtend_so
        ;;
    esac

    copy_system_shared_library libandroid
    copy_system_headers $ANDROID_ROOT/frameworks/base/native/include \
        android/asset_manager.h \
        android/asset_manager_jni.h \
        android/configuration.h \
        android/input.h \
        android/keycodes.h \
        android/looper.h \
        android/native_activity.h \
        android/native_window.h \
        android/native_window_jni.h \
        android/obb.h \
        android/rect.h \
        android/sensor.h \
        android/storage_manager.h \
        android/window.h

    copy_system_shared_library libEGL
    copy_system_headers $ANDROID_ROOT/frameworks/base/opengl/include \
        EGL/egl.h \
        EGL/eglext.h \
        EGL/eglplatform.h

    copy_system_shared_library libOpenSLES
    copy_system_headers $ANDROID_ROOT/$OPENSLES_SUBDIR/include \
        SLES/OpenSLES.h \
        SLES/OpenSLES_Android.h \
        SLES/OpenSLES_AndroidConfiguration.h \
        SLES/OpenSLES_Platform.h
fi

# API level 14
if platform_check 14; then
    copy_system_shared_library libOpenMAXAL
    copy_system_headers $ANDROID_ROOT/system/media/wilhelm/include \
	OMXAL/OpenMAXAL.h \
        OMXAL/OpenMAXAL_Android.h \
        OMXAL/OpenMAXAL_Platform.h

    # This header is new in API level 14
    copy_system_headers $ANDROID_ROOT/$OPENSLES_SUBDIR/include \
        SLES/OpenSLES_AndroidMetadata.h
fi

# Now extract dynamic symbols from the copied shared libraries
# Note that this script will also filter unwanted symbols
log "Generating symbol files"
$PROGDIR/gen-system-symbols.sh "$PLATFORM_ROOT/lib" "$PLATFORM_ROOT/symbols"

# Remove copied shared libraries, we don't need them anymore
# They will be replaced by auto-generated shells by gen-platforms.sh
#
log "Cleaning: $PLATFORM_ROOT/lib/lib*.so"
rm -f "$PLATFORM_ROOT"/lib/lib*.so

log "Done!"
