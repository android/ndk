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

if [ -z "$ANDROID_PRODUCT_OUT" ] ; then
    echo "ERROR: The environment variable ANDROID_PRODUCT_OUT is not defined!"
fi

# This sets HOST_TAG to linux-x86 or darwin-x86 on 64-bit systems
force_32bit_binaries

# We need to extract the auto-detected platform level to display it in
# the help. Do not barf if we can't do it right now, to keep --help working
# in this case.
#
BUILD_PROP=$ANDROID_PRODUCT_OUT/system/build.prop
if [ ! -f "$BUILD_PROP" ] ; then
    # Use this as default if the build.prop file does not exist
    # We will generate an error after the extract_parameters call
    # in this specific case, but we want --help to work before that.
    PLATFORM=9
else
    PLATFORM=`awk -F '=' '$1 == "ro.build.version.sdk" { print $2; }' $BUILD_PROP`
    if [ $? != 0 ] ; then
        dump "WARNING: Could not extract default platform level from $BUILD_PROP!"
        PLATFORM=9
        dump "Defaulting to $PLATFORM"
    fi
fi

PROGRAM_PARAMETERS="<level> [<level2>...]"
PROGRAM_DESCRIPTION=\
"This script is used to update the NDK's platform headers and system libraries
from those generated after a full build of the Android platform. Run it after
modifying and rebuilding one of the public libraries exposed through the NDK.

The first parameter must be a platform/API level. For example, to update the
NDK definitions for the 'android-9' platform, use one of:

    $PROGNAME android-9
    $PROGNAME 9

You can also use several values to update several levels at once, e.g.:

    $PROGNAME $API_LEVELS

NOTE: The currently auto-detected API level for your build is $PLATFORM,
      but this value may be incorrect if your platform has not been assigned
      a new API level yet.

This script is really in charge of the following tasks:

  1/ Import system headers from \$ANDROID/framework/base/ and other
     locations in the full system source tree.

  2/ Locate system shared libraries from \$ANDROID_PRODUCT_OUT/system/lib
     and convert them into small \"shell\" .so files that only export the
     same functions and variables. These can be used with the NDK at link
     time, are much smaller, and also do not require the use of special
     linker flags when used with the standalone NDK toolchain
     (i.e. -Wl,--allow-shlib-undefined)

  3/ For each shared library, also generate a list of symbols exported
     by the shell. This makes it easier to see with 'git diff' which
     symbols were added (or even removed) since the last invocation.

  4/ Copy a few system static libraries (libc.a, libm.a, etc...) used
     to generate static executables. As well as a few key object files
     required by the C runtime (e.g. crtbegin_dynamic.o), when needed.

By default, all files are placed under \$ANDROID/development/ndk
but you can override this with the --out-dir=<path> option.

By default, the build-specific platform/API level is autodetected, and
only the files under \$ANDROID_ROOT/development/ndk/platforms/android-<level>/
will be affected. This ensures you don't accidentally overwrite files
corresponding to previous releases.
"

ARCH=arm
register_var_option "--arch=<name>" ARCH "Specify architecture name."

FORCE=no
register_var_option "--force" FORCE "Force-copy all files."

DEVDIR="$ANDROID_NDK_ROOT/../development/ndk"
if [ -d "$DEVDIR" ] ; then
    OUT_DIR=`cd $DEVDIR && pwd`
else
    OUT_DIR=
fi
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory."

TOOLCHAIN_PREFIX=
register_var_option "--toolchain-prefix=<path>" TOOLCHAIN_PREFIX "Path and prefix for the toolchain"
log "Toolchain prefix: $TOOLCHAIN_PREFIX"

extract_parameters "$@"

if [ -z "$PARAMETERS" ] ; then
    dump "ERROR: Missing required API/platform level. See --help for usage instructions."
    exit 1
fi

# Normalize platform names, i.e. get rid of android- prefix in a list of
# platform levels
#
#  3 android-4 foo-5 -> 3 4 foo-5  (foo-5 is invalid name)
#
# $1: list of platform levels
#
normalize_platforms ()
{
    local RET=$(echo "$@" | tr ' ' '\n' | sed -e 's!android-!!g' | tr '\n' ' ')
    echo ${RET%% }
}

PLATFORMS=`normalize_platforms $PARAMETERS`
log "Target platform levels: $PLATFORMS"

if [ "$FORCE" = "yes" ] ;then
    # We can't accept several platform levels in --force mode.
    NUM_PLATFORMS=$(echo $PLATFORMS | tr ' ' '\n' | wc -l)
    if [ "$NUM_PLATFORMS" != 1 ]; then
        echo "ERROR: You can only use a single platform level when using --force ($NUM_PLATFORMS)"
        exit 1
    fi
fi

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
    if [ "$FORCE" = "yes" ]; then
        PLATFORM_ROOT="$OUT_DIR/platforms/android-$PLATFORMS/arch-$ARCH"
        log "Platform root (forced): $PLATFORM_ROOT"
        return 0
    fi
    echo "$PLATFORMS" | tr ' ' '\n' | fgrep -q "$1"
    if [ $? != 0 ] ; then
        # Not listed, return an error code for 'if'
        return 1
    else
        PLATFORM_ROOT="$OUT_DIR/platforms/android-$1/arch-$ARCH"
        log "Platform root: $PLATFORM_ROOT"
        return 0
    fi
}

# Determine Android build tree root
ANDROID_ROOT=`cd $ANDROID_PRODUCT_OUT/../../../.. && pwd`
log "Android build tree root: $ANDROID_ROOT"
log "Android product out: $ANDROID_PRODUCT_OUT"

case $ARCH in
    arm)
        TOOLCHAIN=$DEFAULT_ARCH_TOOLCHAIN_NAME_arm
        PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm
        ;;
    x86)
        TOOLCHAIN=$DEFAULT_ARCH_TOOLCHAIN_NAME_x86
        PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86
        ;;
    mips)
        TOOLCHAIN=$DEFAULT_ARCH_TOOLCHAIN_NAME_mips
        PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_mips
        ;;
    *)
        echo "ERROR: Unsupported architecture: $ARCH"
        exit 1
esac

if [ -z "$TOOLCHAIN_PREFIX" ]; then
    TOOLCHAIN_NAME=$(get_default_toolchain_name_for_arch $ARCH)
    TOOLCHAIN_PREFIX=$(get_default_toolchain_prefix_for_arch $ARCH)
    TOOLCHAIN_PREFIX=$(get_toolchain_install $ANDROID_NDK_ROOT $TOOLCHAIN_NAME)/bin/$TOOLCHAIN_PREFIX
    TOOLCHAIN_PREFIX=${TOOLCHAIN_PREFIX%%-}
    if [ -z "$TOOLCHAIN_PREFIX" ]; then
        echo "ERROR: Unsupported architecture"
        exit 1
    fi
    echo "Autoconfig: --toolchain-prefix=$TOOLCHAIN_PREFIX"
fi

if [ ! -d "$(dirname $TOOLCHAIN_PREFIX)" ]; then
    echo "ERROR: Toolchain not installed, missing directory: $(dirname $TOOLCHAIN_PREFIX)"
    exit 1
fi

if [ ! -f "$TOOLCHAIN_PREFIX-readelf" ]; then
    echo "ERROR: Toolchain not installed, missing program: $TOOLCHAIN_PREFIX-readelf"
    exit 1
fi

if [ -z "$OUT_DIR" ] ; then
    dump "ERROR: Could not find valid output directory (e.g. \$NDK/../development/ndk)."
    dump "Please use --out-dir=<path> to specify one!"
    exit 1
fi

# Check the platform value and set PLATFORM_ROOT
#

# Normalize the value: android-3 -> android-3   3 -> android-3
PLATFORM=${PLATFORM##android-}
PLATFORM=android-$PLATFORM


# Temp file used to list shared library symbol exclusions
# See set_symbol_excludes and extract_shared_library_xxxx functions below
SYMBOL_EXCLUDES=/tmp/ndk-$USER/ndk-symbol-excludes.txt

# Temp file used to list shared library symbol inclusions, these
# are essentially overrides to the content of SYMBOL_EXCLUDES
SYMBOL_INCLUDES=/tmp/ndk-$USER/ndk-symbol-includes.txt

# Reset the symbol exclusion list to its default
reset_symbol_excludes ()
{
    # By default, do not export C++ mangled symbol, which all start with _Z
    echo '^_Z' > $SYMBOL_EXCLUDES
    > $SYMBOL_INCLUDES
}

# Add new exclusion patterns to SYMBOL_EXCLUDES
set_symbol_excludes ()
{
    (echo "$@" | tr ' ' '\n') >> $SYMBOL_EXCLUDES
}

# Add new inclusion patterns to SYMBOL_INCLUDES
set_symbol_includes ()
{
    (echo "$@" | tr ' ' '\n') >> $SYMBOL_INCLUDES
}

# Clear symbol exclusion/inclusion files
clear_symbol_excludes ()
{
    rm -f $SYMBOL_EXCLUDES $SYMBOL_INCLUDES
}

# Filter the list of symbols from a file
# $1: path to symbol list file
filter_symbols ()
{
    (grep -v -f $SYMBOL_EXCLUDES $1 ; grep -f $SYMBOL_INCLUDES $1) | sort -u
}

#
# Dump the list of dynamic functions exported by a given shared library
# $1: Path to shared library object
extract_shared_library_functions ()
{
    $TOOLCHAIN_PREFIX-readelf -s -D -W $1 | awk '$5 ~ /FUNC/ && $6 ~ /GLOBAL/ && $8 !~ /UND/ { print $9; }' > $TMPC
    filter_symbols $TMPC
}

# Dump the list of global variables exposed by a given shared library
# $1: Path to shared library object
extract_shared_library_variables ()
{
    $TOOLCHAIN_PREFIX-readelf -s -D -W $1 | awk '$5 ~ /OBJECT/ && $6 ~ /GLOBAL/ && $8 !~ /UND/ { print $9; }' > $TMPC
    filter_symbols $TMPC
}

# Generate link library, i.e. a special tiny shell .so that exports the
# same symbols as a reference shared library, and can be used during
# link with the NDK toolchain.
#
# Having these shells allows two things:
#
# - Reduce the size of the NDK release package (some libs are very large)
# - Use the standalone toolchain without -Wl,--allow-shlib-undefined
#
# Note that the list of symbols for each generated library is stored
# under arch-$ARCH/symbols/<libname>.txt
#
# $1: Path to reference shared library
# $2: Path to output shared library (can be the same as $1)
#
generate_shell_library ()
{
    # First, extract the list of functions and variables exported by the
    # reference library.
    local funcs="`extract_shared_library_functions $1`"
    local vars="`extract_shared_library_variables $1`"
    local numfuncs=`echo $funcs | wc -w`
    local numvars=`echo $vars | wc -w`
    dump "Generating shell library for `basename $1` ($numfuncs functions + $numvars variables)"

    # Now generate a small C source file that contains similarly-named stubs
    echo "/* Auto-generated file, do not edit */" > $TMPC
    local func var
    for func in $funcs; do
        echo "void $func(void) {}" >> $TMPC
    done
    for var in $vars; do
        echo "int $var;" >> $TMPC
    done

    # Build it with our cross-compiler. It will complain about conflicting
    # types for built-in functions, so just shut it up.
    $TOOLCHAIN_PREFIX-gcc -Wl,-shared,-Bsymbolic -nostdlib -o $TMPO $TMPC 1>/dev/null 2>&1
    if [ $? != 0 ] ; then
        dump "ERROR: Can't generate shell library for: $1"
        dump "See the content of $TMPC for details."
        exit 1
    fi

    # Sanity check: the generated shared library must export the same
    # functions and variables, or something is really rotten!
    local newfuncs="`extract_shared_library_functions $TMPO`"
    local newvars="`extract_shared_library_variables $TMPO`"
    if [ "$newfuncs" != "$funcs" ] ; then
        dump "ERROR: mismatch in generated functions list"
        exit 1
    fi
    if [ "$newvars" != "$vars" ] ; then
        dump "ERROR: mismatch in generated variables list"
        exit 1
    fi

    # Copy to our destination now
    local libdir=`dirname "$2"`
    mkdir -p "$libdir" && cp -f $TMPO "$2"
    if [ $? != 0 ] ; then
        dump "ERROR: Can't copy shell library for: $1"
        dump "target location is: $2"
        exit 1
    fi

    # Write the functions and variables to a symbols file now
    local symdir=`dirname "$libdir"`/symbols
    local symfile="$symdir/`basename $1.txt`"

    dump "Generating symbol file: $symfile"
    mkdir -p "$symdir" && > $symfile
    (echo "$funcs" | tr ' ' '\n') >>     $symfile
    (echo "$vars" | tr ' ' '\n') >> $symfile

    # Clear the export exclusion list
    reset_symbol_excludes
}

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
    mkdir -p `dirname "$dst"` &&
    generate_shell_library "$src" "$dst"
}

copy_system_static_library ()
{
    local src="$ANDROID_PRODUCT_OUT/obj/STATIC_LIBRARIES/$1_intermediates/$1.a"
    if [ ! -f "$src" ] ; then
        dump "ERROR: Missing system static library: $src"
        exit 1
    fi
    local dst="$PLATFORM_ROOT/lib/$1.a"
    dump "Copying system static library: $1.a"
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
    dump "Copying system object file: $1.o"
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
    dump "Copying system headers from: $srcdir"
    for header; do
        dump "  $header"
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
        dump "Copying $arch system header: $header from $srcdir"
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

# Now do the work

reset_symbol_excludes

# API level 3
if platform_check 3; then
    # Remove a few internal symbols that should not be exposed
    # from the C library (we plan to clean that up soon by using the
    # "hidden" visibility attribute in the near future).
    #
    set_symbol_excludes \
        '^the_' '^dns_' 'load_domain_search_list' 'res_get_dns_changed' \
        '^_resolv_cache' '^_dns_getht' '^_thread_atexit' \
        '^free_malloc_leak_info' 'fake_gmtime_r' 'fake_localtime_r' \
        '^gAllocationsMutex' '^gHashTable' '^gMallocLeakZygoteChild' \

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

    set_symbol_excludes '^.*'         # exclude everything
    set_symbol_includes '^__android_' # except __android_xxxx functions
    copy_system_shared_library liblog
    copy_system_headers $ANDROID_ROOT/system/core/include android/log.h

    # NOTE: We do not copy the C++ headers, they are part of the NDK
    #        under $NDK/source/cxx-stl. They were separated from the rest
    #        of the platform headers in order to make room for other STL
    #        implementations (e.g. STLport or GNU Libstdc++-v3)
    #
    # This is the only library that is allowed to export C++ symbols for now.
    set_symbol_includes '^_Z.*'
    copy_system_shared_library libstdc++
    copy_system_static_library libstdc++

    # We link gdbserver statically with libthreadb, so there is no point
    # in copying the shared library (which by the way has an unstable ABI
    # anyway).
    copy_system_static_library libthread_db
    copy_system_headers $ANDROID_ROOT/bionic/libthread_db/include thread_db.h sys/procfs.h

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

    set_symbol_excludes '^_' '^MPH_' # remove MPH_to_xxx definitions
    copy_system_shared_library libOpenSLES
    copy_system_headers $ANDROID_ROOT/system/media/wilhelm/include \
        SLES/OpenSLES.h \
        SLES/OpenSLES_Android.h \
        SLES/OpenSLES_AndroidConfiguration.h \
        SLES/OpenSLES_AndroidMetadata.h \
        SLES/OpenSLES_Platform.h
fi

# API level 14
if platform_check 14; then
    set_symbol_excludes '^_' '^MPH_' # remove MPH_to_xxx definitions
    copy_system_shared_library libOpenMAXAL
    copy_system_headers $ANDROID_ROOT/system/media/wilhelm/include \
        OMXAL/OpenMAXAL.h \
        OMXAL/OpenMAXAL_Android.h \
        OMXAL/OpenMAXAL_Platform.h
fi

clear_symbol_excludes

dump "Done!"
