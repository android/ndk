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

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
. `dirname $0`/builder-funcs.sh

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Rebuild android support library for the Android NDK.

This requires a temporary NDK installation containing
toolchain binaries for all target architectures.

By default, this will try with the current NDK directory, unless
you use the --ndk-dir=<path> option.

The output will be placed in appropriate sub-directories of
<ndk>/$SUPPORT_SUBDIR, but you can override this with the --out-dir=<path>
option.
"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Put prebuilt tarballs into <path>."

NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "Specify NDK root path for the build."

BUILD_DIR=
OPTION_BUILD_DIR=
register_var_option "--build-dir=<path>" OPTION_BUILD_DIR "Specify temporary build dir."

OUT_DIR=
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory directly."

ABIS="$PREBUILT_ABIS"
register_var_option "--abis=<list>" ABIS "Specify list of target ABIs."

NO_MAKEFILE=
register_var_option "--no-makefile" NO_MAKEFILE "Do not use makefile to speed-up build"

GCC_VERSION=
register_var_option "--gcc-version=<ver>" GCC_VERSION "Specify GCC version"

LLVM_VERSION=
register_var_option "--llvm-version=<ver>" LLVM_VERSION "Specify LLVM version"

register_jobs_option

extract_parameters "$@"

ABIS=$(commas_to_spaces $ABIS)

# Handle NDK_DIR
if [ -z "$NDK_DIR" ] ; then
    NDK_DIR=$ANDROID_NDK_ROOT
    log "Auto-config: --ndk-dir=$NDK_DIR"
else
    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: NDK directory does not exists: $NDK_DIR"
        exit 1
    fi
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    BUILD_DIR=$NDK_TMPDIR/build-support
else
    BUILD_DIR=$OPTION_BUILD_DIR
fi
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
fail_panic "Could not create build directory: $BUILD_DIR"

# Compiler flags we want to use
SUPPORT_CFLAGS="-fPIC -O2 -DANDROID -D__ANDROID__ -ffunction-sections -std=c99"
SUPPORT_CFLAGS=$SUPPORT_CFLAGS" -I$SUPPORT_SUBDIR/include"
SUPPORT_LDFLAGS=""

# android/support files for libc++
SUPPORT32_SOURCES=\
"src/locale_support.c \
src/math_support.c \
src/stdlib_support.c \
src/wchar_support.c \
src/locale/duplocale.c \
src/locale/freelocale.c \
src/locale/localeconv.c \
src/locale/newlocale.c \
src/locale/uselocale.c \
src/stdio/stdio_impl.c \
src/stdio/strtod.c \
src/stdio/vfprintf.c \
src/stdio/vfwprintf.c \
src/msun/e_log2.c \
src/msun/e_log2f.c \
src/msun/s_nan.c \
src/musl-multibyte/btowc.c \
src/musl-multibyte/internal.c \
src/musl-multibyte/mblen.c \
src/musl-multibyte/mbrlen.c \
src/musl-multibyte/mbrtowc.c \
src/musl-multibyte/mbsinit.c \
src/musl-multibyte/mbsnrtowcs.c \
src/musl-multibyte/mbsrtowcs.c \
src/musl-multibyte/mbstowcs.c \
src/musl-multibyte/mbtowc.c \
src/musl-multibyte/wcrtomb.c \
src/musl-multibyte/wcsnrtombs.c \
src/musl-multibyte/wcsrtombs.c \
src/musl-multibyte/wcstombs.c \
src/musl-multibyte/wctob.c \
src/musl-multibyte/wctomb.c \
src/musl-ctype/iswalnum.c \
src/musl-ctype/iswalpha.c \
src/musl-ctype/iswblank.c \
src/musl-ctype/iswcntrl.c \
src/musl-ctype/iswctype.c \
src/musl-ctype/iswdigit.c \
src/musl-ctype/iswgraph.c \
src/musl-ctype/iswlower.c \
src/musl-ctype/iswprint.c \
src/musl-ctype/iswpunct.c \
src/musl-ctype/iswspace.c \
src/musl-ctype/iswupper.c \
src/musl-ctype/iswxdigit.c \
src/musl-ctype/isxdigit.c \
src/musl-ctype/towctrans.c \
src/musl-ctype/wcswidth.c \
src/musl-ctype/wctrans.c \
src/musl-ctype/wcwidth.c \
src/musl-locale/catclose.c \
src/musl-locale/catgets.c \
src/musl-locale/catopen.c \
src/musl-locale/iconv.c \
src/musl-locale/intl.c \
src/musl-locale/isalnum_l.c \
src/musl-locale/isalpha_l.c \
src/musl-locale/isblank_l.c \
src/musl-locale/iscntrl_l.c \
src/musl-locale/isdigit_l.c \
src/musl-locale/isgraph_l.c \
src/musl-locale/islower_l.c \
src/musl-locale/isprint_l.c \
src/musl-locale/ispunct_l.c \
src/musl-locale/isspace_l.c \
src/musl-locale/isupper_l.c \
src/musl-locale/iswalnum_l.c \
src/musl-locale/iswalpha_l.c \
src/musl-locale/iswblank_l.c \
src/musl-locale/iswcntrl_l.c \
src/musl-locale/iswctype_l.c \
src/musl-locale/iswdigit_l.c \
src/musl-locale/iswgraph_l.c \
src/musl-locale/iswlower_l.c \
src/musl-locale/iswprint_l.c \
src/musl-locale/iswpunct_l.c \
src/musl-locale/iswspace_l.c \
src/musl-locale/iswupper_l.c \
src/musl-locale/iswxdigit_l.c \
src/musl-locale/isxdigit_l.c \
src/musl-locale/langinfo.c \
src/musl-locale/strcasecmp_l.c \
src/musl-locale/strcoll.c \
src/musl-locale/strerror_l.c \
src/musl-locale/strfmon.c \
src/musl-locale/strftime_l.c \
src/musl-locale/strncasecmp_l.c \
src/musl-locale/strxfrm.c \
src/musl-locale/tolower_l.c \
src/musl-locale/toupper_l.c \
src/musl-locale/towctrans_l.c \
src/musl-locale/towlower_l.c \
src/musl-locale/towupper_l.c \
src/musl-locale/wcscoll.c \
src/musl-locale/wcsxfrm.c \
src/musl-locale/wctrans_l.c \
src/musl-locale/wctype_l.c \
src/musl-math/frexpf.c \
src/musl-math/frexpl.c \
src/musl-math/frexp.c \
src/musl-stdio/swprintf.c \
src/musl-stdio/vwprintf.c \
src/musl-stdio/wprintf.c \
src/musl-stdio/printf.c \
src/musl-stdio/snprintf.c \
src/musl-stdio/sprintf.c \
src/musl-stdio/vprintf.c \
src/musl-stdio/vsprintf.c \
src/wcstox/intscan.c \
src/wcstox/floatscan.c \
src/wcstox/shgetc.c \
src/wcstox/wcstod.c \
src/wcstox/wcstol.c \
"
# Replaces broken implementations in x86 libm.so
SUPPORT32_SOURCES_x86=\
"src/musl-math/scalbln.c \
src/musl-math/scalblnf.c \
src/musl-math/scalblnl.c \
src/musl-math/scalbnl.c \
"

# android/support files for libc++
SUPPORT64_SOURCES=\
"src/musl-locale/catclose.c \
src/musl-locale/catgets.c \
src/musl-locale/catopen.c \
"

# If the --no-makefile flag is not used, we're going to put all build
# commands in a temporary Makefile that we will be able to invoke with
# -j$NUM_JOBS to build stuff in parallel.
#
if [ -z "$NO_MAKEFILE" ]; then
    MAKEFILE=$BUILD_DIR/Makefile
else
    MAKEFILE=
fi

# build_support_libs_for_abi
# $1: ABI
# $2: build directory
# $3: (optional) installation directory
build_support_libs_for_abi ()
{
    local ARCH BINPREFIX
    local ABI=$1
    local BUILDDIR="$2"
    local DSTDIR="$3"
    local GCCVER

    mkdir -p "$BUILDDIR"

    # If the output directory is not specified, use default location
    if [ -z "$DSTDIR" ]; then
        DSTDIR=$NDK_DIR/$SUPPORT_SUBDIR/libs/$ABI
    fi

    mkdir -p "$DSTDIR"
    ARCH=$(convert_abi_to_arch $ABI)
    if [ -n "$GCC_VERSION" ]; then
        GCCVER=$GCC_VERSION
    else
        GCCVER=$(get_default_gcc_version_for_arch $ARCH)
    fi

    builder_begin_android $ABI "$BUILDDIR" "$GCCVER" "$LLVM_VERSION" "$MAKEFILE"
    builder_set_srcdir "$SUPPORT_SUBDIR"
    builder_set_dstdir "$DSTDIR"

    builder_cflags "$SUPPORT_CFLAGS"

    if [ $ABI = "armeabi-v7a-hard" ]; then
        builder_cflags "-mhard-float -D_NDK_MATH_NO_SOFTFP=1"
    fi

    builder_ldflags "$SUPPORT_LDFLAGS"
    if [ $ABI = "armeabi-v7a-hard" ]; then
        builder_cflags "-Wl,--no-warn-mismatch -lm_hard"
    fi

    if [ "$ABI" = "${ABI%%64*}" ]; then
        if [ "$ABI" = "x86" ]; then
            builder_sources $SUPPORT32_SOURCES $SUPPORT32_SOURCES_x86
        else
            builder_sources $SUPPORT32_SOURCES
        fi
    else
        builder_sources $SUPPORT64_SOURCES
    fi

    log "Building $DSTDIR/libandroid_support.a"
    builder_static_library libandroid_support

    builder_end
}

for ABI in $ABIS; do
    build_support_libs_for_abi $ABI "$BUILD_DIR/$ABI" "$OUT_DIR"
done

# If needed, package files into tarballs
if [ -n "$PACKAGE_DIR" ] ; then
    for ABI in $ABIS; do
        FILES="$SUPPORT_SUBDIR/libs/$ABI/libandroid_support.a"
        PACKAGE="$PACKAGE_DIR/support-libs-$ABI.tar.bz2"
        log "Packaging: $PACKAGE"
        pack_archive "$PACKAGE" "$NDK_DIR" "$FILES"
        fail_panic "Could not package $ABI support binary!"
        dump "Packaging: $PACKAGE"
    done
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    log "Cleaning up..."
    rm -rf $BUILD_DIR
else
    log "Don't forget to cleanup: $BUILD_DIR"
fi

log "Done!"
