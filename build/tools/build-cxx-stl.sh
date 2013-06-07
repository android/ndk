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
#  This shell script is used to rebuild one of the NDK C++ STL
#  implementations from sources. To use it:
#
#   - Define CXX_STL to one of 'gabi++', 'stlport' or 'libc++'
#   - Run it.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
. `dirname $0`/builder-funcs.sh

CXX_STL_LIST="gabi++ stlport libc++"

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Rebuild one of the following NDK C++ runtimes: $CXX_STL_LIST.

This script is called when pacakging a new NDK release. It will simply
rebuild the static and shared libraries of a given C++ runtime from
sources.

Use the --stl=<name> option to specify which runtime you want to rebuild.

This requires a temporary NDK installation containing platforms and
toolchain binaries for all target architectures.

By default, this will try with the current NDK directory, unless
you use the --ndk-dir=<path> option.

If you want to use clang to rebuild the binaries, please use
--llvm-version=<ver> option.

The output will be placed in appropriate sub-directories of
<ndk>/sources/cxx-stl/$CXX_STL_SUBDIR, but you can override this with
the --out-dir=<path> option.
"

CXX_STL=
register_var_option "--stl=<name>" CXX_STL "Select C++ runtime to rebuild."

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

VISIBLE_STATIC=
register_var_option "--visible-static" VISIBLE_STATIC "Do not use hidden visibility for the static library"

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
  if [ ! -d "$NDK_DIR" ]; then
    panic "NDK directory does not exist: $NDK_DIR"
  fi
fi

# Handle OUT_DIR
if [ -z "$OUT_DIR" ] ; then
  OUT_DIR=$ANDROID_NDK_ROOT
  log "Auto-config: --out-dir=$OUT_DIR"
else
  mkdir -p "$OUT_DIR"
  fail_panic "Could not create directory: $OUT_DIR"
fi

# Check that --stl=<name> is used with one of the supported runtime names.
if [ -z "$CXX_STL" ]; then
  panic "Please use --stl=<name> to select a C++ runtime to rebuild."
fi
FOUND=
for STL in $CXX_STL_LIST; do
  if [ "$STL" = "$CXX_STL" ]; then
    FOUND=true
    break
  fi
done
if [ -z "$FOUND" ]; then
  panic "Invalid --stl value ('$CXX_STL'), please use one of: $CXX_STL_LIST."
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    BUILD_DIR=$NDK_TMPDIR/build-$CXX_STL
else
    BUILD_DIR=$OPTION_BUILD_DIR
fi
mkdir -p "$BUILD_DIR"
fail_panic "Could not create build directory: $BUILD_DIR"

# Location of the various C++ runtime source trees.
GABIXX_SRCDIR=$ANDROID_NDK_ROOT/$GABIXX_SUBDIR
STLPORT_SRCDIR=$ANDROID_NDK_ROOT/$STLPORT_SUBDIR
LIBCXX_SRCDIR=$ANDROID_NDK_ROOT/$LIBCXX_SUBDIR

LIBCXX_INCLUDES="-I$LIBCXX_SRCDIR/libcxx/include -I$LIBCXX_SRCDIR/android/support/include -I$GABIXX_SRCDIR/include"

COMMON_CFLAGS="-fPIC -O2 -ffunction-sections"
COMMON_CXXFLAGS="-fexceptions -frtti -fuse-cxa-atexit"

# Determine GAbi++ build parameters. Note that GAbi++ is also built as part
# of STLport and Libc++, in slightly different ways.
if [ "$CXX_STL" = "libc++" ]; then
  GABIXX_INCLUDES=$LIBCXX_INCLUDES
else
  GABIXX_INCLUDES="-I$GABIXX_SRCDIR/include"
fi
GABIXX_CFLAGS="$COMMON_CFLAGS $GABIXX_INCLUDES"
GABIXX_CXXFLAGS="$COMMON_CXXFLAGS"
GABIXX_SOURCES=$(cd $ANDROID_NDK_ROOT/$GABIXX_SUBDIR && ls src/*.cc)
GABIXX_LDFLAGS="-ldl"
if [ "$CXX_STL" = "libc++" ]; then
  GABIXX_CXXFLAGS="$GABIXX_CXXFLAGS -DGABIXX_LIBCXX=1"
fi

# Determine STLport build parameters
STLPORT_CFLAGS="$COMMON_CFLAGS -DGNU_SOURCE -I$STLPORT_SRCDIR/stlport $GABIXX_INCLUDES"
STLPORT_CXXFLAGS="$COMMON_CXXFLAGS"
STLPORT_SOURCES=\
"src/dll_main.cpp \
src/fstream.cpp \
src/strstream.cpp \
src/sstream.cpp \
src/ios.cpp \
src/stdio_streambuf.cpp \
src/istream.cpp \
src/ostream.cpp \
src/iostream.cpp \
src/codecvt.cpp \
src/collate.cpp \
src/ctype.cpp \
src/monetary.cpp \
src/num_get.cpp \
src/num_put.cpp \
src/num_get_float.cpp \
src/num_put_float.cpp \
src/numpunct.cpp \
src/time_facets.cpp \
src/messages.cpp \
src/locale.cpp \
src/locale_impl.cpp \
src/locale_catalog.cpp \
src/facets_byname.cpp \
src/complex.cpp \
src/complex_io.cpp \
src/complex_trig.cpp \
src/string.cpp \
src/bitset.cpp \
src/allocators.cpp \
src/c_locale.c \
src/cxa.c"

# Determine Libc++ build parameters
LIBCXX_CFLAGS="$COMMON_CFLAGS $LIBCXX_INCLUDES"
LIBCXX_CXXFLAGS="$COMMON_CXXFLAGS -DLIBCXXRT=1 -DGABIXX_LIBCXX=1 -std=c++11"
LIBCXX_SOURCES=\
"libcxx/src/algorithm.cpp \
libcxx/src/bind.cpp \
libcxx/src/chrono.cpp \
libcxx/src/condition_variable.cpp \
libcxx/src/debug.cpp \
libcxx/src/exception.cpp \
libcxx/src/future.cpp \
libcxx/src/hash.cpp \
libcxx/src/ios.cpp \
libcxx/src/iostream.cpp \
libcxx/src/locale.cpp \
libcxx/src/memory.cpp \
libcxx/src/mutex.cpp \
libcxx/src/new.cpp \
libcxx/src/random.cpp \
libcxx/src/regex.cpp \
libcxx/src/stdexcept.cpp \
libcxx/src/string.cpp \
libcxx/src/strstream.cpp \
libcxx/src/system_error.cpp \
libcxx/src/thread.cpp \
libcxx/src/typeinfo.cpp \
libcxx/src/utility.cpp \
libcxx/src/valarray.cpp \
android/support/src/locale_support.c \
android/support/src/nl_types_support.c \
android/support/src/stdlib_support.c \
android/support/src/wchar_support.c \
android/support/src/wctype_support.c"


# If the --no-makefile flag is not used, we're going to put all build
# commands in a temporary Makefile that we will be able to invoke with
# -j$NUM_JOBS to build stuff in parallel.
#
if [ -z "$NO_MAKEFILE" ]; then
    MAKEFILE=$BUILD_DIR/Makefile
else
    MAKEFILE=
fi

# Define a few common variables based on parameters.
case $CXX_STL in
  gabi++)
    CXX_STL_SUBDIR=$GABIXX_SUBDIR
    CXX_STL_SRCDIR=$GABIXX_SRCDIR
    CXX_STL_CFLAGS=$GABIXX_CFLAGS
    CXX_STL_CXXFLAGS=$GABIXX_CXXFLAGS
    CXX_STL_LDFLAGS=$GABIXX_LDFLAGS
    CXX_STL_SOURCES=$GABIXX_SOURCES
    CXX_STL_PACKAGE=gabixx
    ;;
  stlport)
    CXX_STL_SUBDIR=$STLPORT_SUBDIR
    CXX_STL_SRCDIR=$STLPORT_SRCDIR
    CXX_STL_CFLAGS=$STLPORT_CFLAGS
    CXX_STL_CXXFLAGS=$STLPORT_CXXFLAGS
    CXX_STL_LDFLAGS=$STLPORT_LDFLAGS
    CXX_STL_SOURCES=$STLPORT_SOURCES
    CXX_STL_PACKAGE=stlport
    ;;
  libc++)
    CXX_STL_SUBDIR=$LIBCXX_SUBDIR
    CXX_STL_SRCDIR=$LIBCXX_SRCDIR
    CXX_STL_CFLAGS=$LIBCXX_CFLAGS
    CXX_STL_CXXFLAGS=$LIBCXX_CXXFLAGS
    CXX_STL_LDFLAGS=$LIBCXX_LDFLAGS
    CXX_STL_SOURCES=$LIBCXX_SOURCES
    CXX_STL_PACKAGE=libcxx
    ;;
  *)
    panic "Internal error: Unknown STL name '$CXX_STL'"
    ;;
esac

# By default, all static libraries include hidden ELF symbols, except
# if one uses the --visible-static option.
if [ -z "$VISIBLE_STATIC" ]; then
  STATIC_CXXFLAGS="-fvisibility=hidden -fvisibility-inlines-hidden"
else
  STATIC_CXXFLAGS=
fi
SHARED_CXXFLAGS=

# build_stl_libs_for_abi
# $1: ABI
# $2: build directory
# $3: build type: "static" or "shared"
# $4: (optional) installation directory
build_stl_libs_for_abi ()
{
    local ARCH BINPREFIX SYSROOT
    local ABI=$1
    local BUILDDIR="$2"
    local TYPE="$3"
    local DSTDIR="$4"
    local DEFAULT_CFLAGS DEFAULT_CXXFLAGS
    local SRC OBJ OBJECTS EXTRA_CXXFLAGS

    mkdir -p "$BUILDDIR"

    DSTDIR=$DSTDIR/$CXX_STL_SUBDIR/libs/$ABI

    if [ "$TYPE" = "static" -a -z "$VISIBLE_STATIC" ]; then
      EXTRA_CXXFLAGS="$STATIC_CXXFLAGS"
    else
      EXTRA_CXXFLAGS="$SHARED_CXXFLAGS"
    fi

    mkdir -p "$DSTDIR"

    builder_begin_android $ABI "$BUILDDIR" "$LLVM_VERSION" "$MAKEFILE"

    builder_set_dstdir "$DSTDIR"

    # Always rebuild GAbi++, except for unknown archs.
    if [ "$CXX_STL" = "gabi++" -o "$(arch_in_unknown_archs $ABI)" != "yes" ]; then
      builder_set_srcdir "$GABIXX_SRCDIR"
      builder_reset_cflags DEFAULT_CFLAGS
      builder_cflags "$DEFAULT_CFLAGS $GABIXX_CFLAGS"

      builder_reset_cxxflags DEFAULT_CXXFLAGS
      builder_cxxflags "$DEFAULT_CXXFLAGS $GABIXX_CXXFLAGS $EXTRA_CXXFLAGS"
      builder_ldflags "$GABIXX_LDFLAGS"
      builder_sources $GABIXX_SOURCES
    fi

    # Build the runtime sources, except if we're only building GAbi++
    if [ "$CXX_STL" != "gabi++" ]; then
      builder_set_srcdir "$CXX_STL_SRCDIR"
      builder_reset_cflags
      builder_cflags "$DEFAULT_CFLAGS $CXX_STL_CFLAGS"
      builder_reset_cxxflags DEFAULT_CXXFLAGS
      builder_cxxflags "$DEFAULT_CXXFLAGS $CXX_STL_CXXFLAGS $EXTRA_CXXFLAGS"
      builder_ldflags "$CXX_STL_LDFLAGS"
      builder_sources $CXX_STL_SOURCES
    fi

    if [ "$TYPE" = "static" ]; then
        log "Building $DSTDIR/${CXX_STL}_static.a"
        builder_static_library ${CXX_STL}_static
    else
        log "Building $DSTDIR/${CXX_STL}_shared.so"
        builder_shared_library ${CXX_STL}_shared
    fi

    builder_end
}

for ABI in $ABIS; do
    build_stl_libs_for_abi $ABI "$BUILD_DIR/$ABI/shared" "shared" "$OUT_DIR"
    build_stl_libs_for_abi $ABI "$BUILD_DIR/$ABI/static" "static" "$OUT_DIR"
done

# If needed, package files into tarballs
if [ -n "$PACKAGE_DIR" ] ; then
    for ABI in $ABIS; do
        FILES=""
        for LIB in ${CXX_STL}_static.a ${CXX_STL}_shared.so; do
            FILES="$FILES $CXX_STL_SUBDIR/libs/$ABI/$LIB"
        done
        PACKAGE="$PACKAGE_DIR/${CXX_STL_PACKAGE}-libs-$ABI.tar.bz2"
        log "Packaging: $PACKAGE"
        pack_archive "$PACKAGE" "$OUT_DIR" "$FILES"
        fail_panic "Could not package $ABI $CXX_STL binaries!"
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
