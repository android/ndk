#!/bin/sh
#
# Copyright (C) 2011 The Android Open Source Project
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
#  This shell script is used to rebuild the prebuilt GNU libsupc++ and
#  libstdc++ binaries from their sources. It requires an NDK installation
#  that contains valid plaforms files and toolchain binaries.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir>"

PROGRAM_DESCRIPTION=\
"Rebuild the prebuilt GNU libsupc++ / libstdc++ binaries for the Android NDK.

This script is called when packaging a new NDK release. It will simply
rebuild the GNU libsupc++ and libstdc++ static and shared libraries from
sources.

This requires a temporary NDK installation containing platforms and
toolchain binaries for all target architectures, as well as the path to
the corresponding gcc source tree.

By default, this will try with the current NDK directory, unless
you use the --ndk-dir=<path> option.

The output will be placed in appropriate sub-directories of
<ndk>/$GNUSTL_SUBDIR, but you can override this with the --out-dir=<path>
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

ABIS=$(spaces_to_commas $PREBUILT_ABIS)
register_var_option "--abis=<list>" ABIS "Specify list of target ABIs."

JOBS="$BUILD_NUM_CPUS"
register_var_option "-j<number>" JOBS "Use <number> build jobs in parallel"

NO_MAKEFILE=
register_var_option "--no-makefile" NO_MAKEFILE "Do not use makefile to speed-up build"

NUM_JOBS=$BUILD_NUM_CPUS
register_var_option "-j<number>" NUM_JOBS "Run <number> build jobs in parallel"

extract_parameters "$@"

SRCDIR=$(echo $PARAMETERS | sed 1q)
check_toolchain_src_dir "$SRCDIR"

GNUSTL_SRCDIR=$SRCDIR/gcc/gcc-$DEFAULT_GCC_VERSION/libstdc++-v3
if [ ! -d "$GNUSTL_SRCDIR" ]; then
    echo "ERROR: Not a valid toolchain source tree."
    echo "Can't find: $GNUSTL_SRCDIR"
    exit 1
fi

if [ ! -f "$GNUSTL_SRCDIR/configure" ]; then
    echo "ERROR: Configure script missing: $GNUSTL_SRCDIR/configure"
    exit 1
fi

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
    BUILD_DIR=$NDK_TMPDIR/build-gnustl
else
    BUILD_DIR=$OPTION_BUILD_DIR
fi
mkdir -p "$BUILD_DIR"
fail_panic "Could not create build directory: $BUILD_DIR"

# $1: ABI name
# $2: Build directory
# $3: "static" or "shared"
# $4: Destination directory (optional, will default to $GNUSTL_SUBDIR/lib/$ABI)
build_gnustl_for_abi ()
{
    local ARCH BINPREFIX SYSROOT
    local ABI=$1
    local BUILDDIR="$2"
    local LIBTYPE="$3"
    local DSTDIR="$4"
    local SRC OBJ OBJECTS CFLAGS CXXFLAGS

    prepare_target_build $ABI $PLATFORM $NDK_DIR
    fail_panic "Could not setup target build."

    INSTALLDIR=$BUILDDIR/install
    BUILDDIR=$BUILDDIR/$LIBTYPE-$ABI

    # If the output directory is not specified, use default location
    if [ -z "$DSTDIR" ]; then
        DSTDIR=$NDK_DIR/$GNUSTL_SUBDIR/libs/$ABI
    fi
    mkdir -p $DSTDIR

    ARCH=$(convert_abi_to_arch $ABI)
    BINPREFIX=$NDK_DIR/$(get_default_toolchain_binprefix_for_arch $ARCH)
    SYSROOT=$NDK_DIR/$(get_default_platform_sysroot_for_arch $ARCH)

    # Sanity check
    if [ ! -f "$SYSROOT/usr/lib/libc.a" ]; then
	echo "ERROR: Empty sysroot! you probably need to run gen-platforms.sh before this script."
	exit 1
    fi
    if [ ! -f "$SYSROOT/usr/lib/libc.so" ]; then
        echo "ERROR: Sysroot misses shared libraries! you probably need to run gen-platforms.sh"
        echo "*without* the --minimal flag before running this script."
        exit 1
    fi 

    case $ARCH in
        arm)
            BUILD_HOST=arm-linux-androideabi
            ;;
        x86)
            BUILD_HOST=i686-android-linux
            ;;
    esac

    export CXXFLAGS="$CXXFLAGS --sysroot=$SYSROOT -fexceptions -frtti -D__BIONIC__ -O2"

    export CC=${BINPREFIX}gcc
    export CXX=${BINPREFIX}g++
    export AS=${BINPREFIX}as
    export LD=${BINPREFIX}ld
    export AR=${BINPREFIX}ar
    export RANLIB=${BINPREFIX}ranlib
    export STRIP=${BINPREFIX}strip

    setup_ccache

    export LDFLAGS="-nostdinc -L$SYSROOT/usr/lib -lc"

    LIBTYPE_FLAGS=
    if [ $LIBTYPE = "static" ]; then
        # Ensure we disable visibility for the static library to reduce the
        # size of the code that will be linked against it.
        LIBTYPE_FLAG="--enable-static --disable-shared --disable-visibility"
        CXXFLAGS=$CXXFLAGS" -fvisibility=hidden -fvisibility-inlines-hidden"
    else
        LIBTYPE_FLAG="--disable-static --enable-shared"
        #LDFLAGS=$LDFLAGS" -lsupc++"
    fi

    PROJECT="gnustl_$LIBTYPE $ABI"
    echo "$PROJECT: configuring"
    mkdir -p $BUILDDIR && rm -rf $BUILDDIR/* &&
    cd $BUILDDIR &&
    run $GNUSTL_SRCDIR/configure \
        --prefix=$INSTALLDIR \
        --host=$BUILD_HOST \
        $LIBTYPE_FLAGS \
        --disable-symvers \
        --disable-multilib \
        --enable-threads \
        --disable-nls \
        --disable-sjlj-exceptions \
        --disable-tls \
        --disable-libstdcxx-pch

    fail_panic "Could not configure $PROJECT"

    echo "$PROJECT: compiling"
    run make -j$NUM_JOBS
    fail_panic "Could not build $PROJECT"

    echo "$PROJECT: installing"
    run make install
    fail_panic "Could not create $ABI prebuilts for GNU libsupc++/libstdc++"
}


HAS_COMMON_HEADERS=

# $1: ABI
# $2: Build directory
copy_gnustl_libs ()
{
    local ABI="$1"
    local BUILDDIR="$2"
    local ARCH=$(convert_abi_to_arch $ABI)
    local VERSION=$DEFAULT_GCC_VERSION
    local PREFIX=$(get_default_toolchain_prefix_for_arch $ARCH)
    PREFIX=${PREFIX%%-}

    local SDIR="$BUILDDIR/install"
    local DDIR="$NDK_DIR/$GNUSTL_SUBDIR"

    # Copy the common headers only the first time this function is called.
    if [ -z "$HAS_COMMON_HEADERS" ]; then
        copy_directory "$SDIR/include/c++/$VERSION" "$DDIR/include"
        rm -rf "$DDIR/include/$PREFIX"
        HAS_COMMON_HEADERS=true
    fi

    rm -rf "$DIR/libs/$ABI" && 
    mkdir -p "$DDIR/libs/$ABI/include"

    # Copy the ABI-specific headers
    copy_directory "$SDIR/include/c++/$VERSION/$PREFIX/bits" "$DDIR/libs/$ABI/include/bits"

    # Copy the ABI-specific libraries
    # Note: the shared library name is libgnustl_shared.so due our custom toolchain patch
    # We need to copy libstdc++.so which is identical to libgnustl_shared.so except for the DT_LIBRARY entry
    # within the ELF file, since it will be needed by the standalone toolchain installation later.
    copy_file_list "$SDIR/lib" "$DDIR/libs/$ABI" libsupc++.a libstdc++.so libgnustl_shared.so
    # Note: we need to rename libgnustl_shared.a to libgnustl_static.a
    cp "$SDIR/lib/libgnustl_shared.a" "$DDIR/libs/$ABI/libgnustl_static.a"
}



for ABI in $ABIS; do
    build_gnustl_for_abi $ABI "$BUILD_DIR" static
    build_gnustl_for_abi $ABI "$BUILD_DIR" shared
    copy_gnustl_libs $ABI "$BUILD_DIR"
done

# If needed, package files into tarballs
if [ -n "$PACKAGE_DIR" ] ; then
    # First, the headers as a single package
    PACKAGE="$PACKAGE_DIR/gnu-libstdc++-headers.tar.bz2"
    dump "Packaging: $PACKAGE"
    pack_archive "$PACKAGE" "$NDK_DIR" "$GNUSTL_SUBDIR/include"

    # Then, one package per ABI for libraries
    for ABI in $ABIS; do
        FILES=""
        for LIB in include/bits libsupc++.a libgnustl_static.a libstdc++.so libgnustl_shared.so; do
            FILES="$FILES $GNUSTL_SUBDIR/libs/$ABI/$LIB"
        done
        PACKAGE="$PACKAGE_DIR/gnu-libstdc++-libs-$ABI.tar.bz2"
        dump "Packaging: $PACKAGE"
        pack_archive "$PACKAGE" "$NDK_DIR" "$FILES"
        fail_panic "Could not package $ABI STLport binaries!"
    done
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    log "Cleaning up..."
    rm -rf $BUILD_DIR
else
    log "Don't forget to cleanup: $BUILD_DIR"
fi

log "Done!"
