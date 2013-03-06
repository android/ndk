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
<ndk>/$GNUSTL_SUBDIR/<gcc-version>, but you can override this with the --out-dir=<path>
option.
"
GCC_VERSION_LIST=$DEFAULT_GCC_VERSION_LIST
register_var_option "--gcc-version-list=<vers>" GCC_VERSION_LIST "List of GCC versions"

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

NO_MAKEFILE=
register_var_option "--no-makefile" NO_MAKEFILE "Do not use makefile to speed-up build"

VISIBLE_LIBGNUSTL_STATIC=
register_var_option "--visible-libgnustl-static" VISIBLE_LIBGNUSTL_STATIC "Do not use hidden visibility for libgnustl_static.a"

register_jobs_option

extract_parameters "$@"

SRCDIR=$(echo $PARAMETERS | sed 1q)
check_toolchain_src_dir "$SRCDIR"

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
# $4: GCC version
# $5: optional "thumb"
build_gnustl_for_abi ()
{
    local ARCH BINPREFIX SYSROOT GNUSTL_SRCDIR
    local ABI=$1
    local BUILDDIR="$2"
    local LIBTYPE="$3"
    local GCC_VERSION="$4"
    local THUMB="$5"
    local DSTDIR=$NDK_DIR/$GNUSTL_SUBDIR/$GCC_VERSION/libs/$ABI/$THUMB
    local SRC OBJ OBJECTS CFLAGS CXXFLAGS

    prepare_target_build $ABI $PLATFORM $NDK_DIR
    fail_panic "Could not setup target build."

    INSTALLDIR=$BUILDDIR/install-$ABI-$GCC_VERSION/$THUMB
    BUILDDIR=$BUILDDIR/$LIBTYPE-${ABI}${THUMB}-$GCC_VERSION

    mkdir -p $DSTDIR

    ARCH=$(convert_abi_to_arch $ABI)
    BINPREFIX=$NDK_DIR/$(get_toolchain_binprefix_for_arch $ARCH $GCC_VERSION)

    GNUSTL_SRCDIR=$SRCDIR/gcc/gcc-$GCC_VERSION/libstdc++-v3
    # Sanity check
    if [ ! -d "$GNUSTL_SRCDIR" ]; then
        echo "ERROR: Not a valid toolchain source tree."
        echo "Can't find: $GNUSTL_SRCDIR"
        exit 1
    fi

    if [ ! -f "$GNUSTL_SRCDIR/configure" ]; then
        echo "ERROR: Configure script missing: $GNUSTL_SRCDIR/configure"
        exit 1
    fi

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
            BUILD_HOST=i686-linux-android
            ;;
        mips)
            BUILD_HOST=mipsel-linux-android
            ;;
    esac

    EXTRA_FLAGS=
    if [ -n "$THUMB" ] ; then
        EXTRA_FLAGS="-mthumb"
    fi
    export CFLAGS="-fPIC $CFLAGS --sysroot=$SYSROOT -fexceptions -funwind-tables -D__BIONIC__ -O2 $EXTRA_FLAGS"
    export CXXFLAGS="-fPIC $CXXFLAGS --sysroot=$SYSROOT -fexceptions -frtti -funwind-tables -D__BIONIC__ -O2 $EXTRA_FLAGS"
    export CPPFLAGS="$CPPFLAGS --sysroot=$SYSROOT"

    export CC=${BINPREFIX}gcc
    export CXX=${BINPREFIX}g++
    export AS=${BINPREFIX}as
    export LD=${BINPREFIX}ld
    export AR=${BINPREFIX}ar
    export RANLIB=${BINPREFIX}ranlib
    export STRIP=${BINPREFIX}strip

    setup_ccache

    export LDFLAGS="-L$SYSROOT/usr/lib -lc $EXTRA_FLAGS"

    if [ "$ABI" = "armeabi-v7a" ]; then
        CXXFLAGS=$CXXFLAGS" -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16"
        LDFLAGS=$LDFLAGS" -Wl,--fix-cortex-a8"
    fi

    LIBTYPE_FLAGS=
    if [ $LIBTYPE = "static" ]; then
        # Ensure we disable visibility for the static library to reduce the
        # size of the code that will be linked against it.
        if [ -z "$VISIBLE_LIBGNUSTL_STATIC" ] ; then
            LIBTYPE_FLAGS="--enable-static --disable-shared"
            if [ $GCC_VERSION = "4.4.3" -o $GCC_VERSION = "4.6" ]; then
                LIBTYPE_FLAGS=$LIBTYPE_FLAGS" --disable-visibility"
            else
                LIBTYPE_FLAGS=$LIBTYPE_FLAGS" --disable-libstdcxx-visibility"
            fi
            CXXFLAGS=$CXXFLAGS" -fvisibility=hidden -fvisibility-inlines-hidden"
        fi
    else
        LIBTYPE_FLAGS="--disable-static --enable-shared"
        #LDFLAGS=$LDFLAGS" -lsupc++"
    fi

    PROJECT="gnustl_$LIBTYPE gcc-$GCC_VERSION $ABI $THUMB"
    echo "$PROJECT: configuring"
    mkdir -p $BUILDDIR && rm -rf $BUILDDIR/* &&
    cd $BUILDDIR &&
    run $GNUSTL_SRCDIR/configure \
        --prefix=$INSTALLDIR \
        --host=$BUILD_HOST \
        $LIBTYPE_FLAGS \
        --enable-libstdcxx-time \
        --disable-symvers \
        --disable-multilib \
        --disable-nls \
        --disable-sjlj-exceptions \
        --disable-tls \
        --disable-libstdcxx-pch \
        --with-gxx-include-dir=$INSTALLDIR/include/c++/$GCC_VERSION

    fail_panic "Could not configure $PROJECT"

    echo "$PROJECT: compiling"
    run make -j$NUM_JOBS
    fail_panic "Could not build $PROJECT"

    echo "$PROJECT: installing"
    run make install
    fail_panic "Could not create $ABI $THUMB prebuilts for GNU libsupc++/libstdc++"
}


HAS_COMMON_HEADERS=

# $1: ABI
# $2: Build directory
# $3: GCC_VERSION
copy_gnustl_libs ()
{
    local ABI="$1"
    local BUILDDIR="$2"
    local ARCH=$(convert_abi_to_arch $ABI)
    local GCC_VERSION="$3"
    local PREFIX=$(get_default_toolchain_prefix_for_arch $ARCH)
    PREFIX=${PREFIX%%-}

    local SDIR="$BUILDDIR/install-$ABI-$GCC_VERSION"
    local DDIR="$NDK_DIR/$GNUSTL_SUBDIR/$GCC_VERSION"

    local GCC_VERSION_NO_DOT=$(echo $GCC_VERSION|sed 's/\./_/g')
    # Copy the common headers only once per gcc version
    if [ -z `var_value HAS_COMMON_HEADERS_$GCC_VERSION_NO_DOT` ]; then
        copy_directory "$SDIR/include/c++/$GCC_VERSION" "$DDIR/include"
        rm -rf "$DDIR/include/$PREFIX"
	eval HAS_COMMON_HEADERS_$GCC_VERSION_NO_DOT=true
    fi

    rm -rf "$DDIR/libs/$ABI" &&
    mkdir -p "$DDIR/libs/$ABI/include"

    # Copy the ABI-specific headers
    copy_directory "$SDIR/include/c++/$GCC_VERSION/$PREFIX/bits" "$DDIR/libs/$ABI/include/bits"

    # Copy the ABI-specific libraries
    # Note: the shared library name is libgnustl_shared.so due our custom toolchain patch
    copy_file_list "$SDIR/lib" "$DDIR/libs/$ABI" libsupc++.a libgnustl_shared.so
    # Note: we need to rename libgnustl_shared.a to libgnustl_static.a
    cp "$SDIR/lib/libgnustl_shared.a" "$DDIR/libs/$ABI/libgnustl_static.a"
    if [ -d "$SDIR/thumb" ] ; then
        copy_file_list "$SDIR/thumb/lib" "$DDIR/libs/$ABI/thumb" libsupc++.a libgnustl_shared.so
        cp "$SDIR/thumb/lib/libgnustl_shared.a" "$DDIR/libs/$ABI/thumb/libgnustl_static.a"
    fi
}

GCC_VERSION_LIST=$(commas_to_spaces $GCC_VERSION_LIST)
for VERSION in $GCC_VERSION_LIST; do
    for ABI in $ABIS; do
        build_gnustl_for_abi $ABI "$BUILD_DIR" static $VERSION
        build_gnustl_for_abi $ABI "$BUILD_DIR" shared $VERSION
        if [ "$ABI" != "${ABI%%arm*}" ] ; then
            build_gnustl_for_abi $ABI "$BUILD_DIR" static $VERSION thumb
            build_gnustl_for_abi $ABI "$BUILD_DIR" shared $VERSION thumb
        fi
        copy_gnustl_libs $ABI "$BUILD_DIR" $VERSION
    done
done

# If needed, package files into tarballs
if [ -n "$PACKAGE_DIR" ] ; then
    for VERSION in $GCC_VERSION_LIST; do
        # First, the headers as a single package for a given gcc version
        PACKAGE="$PACKAGE_DIR/gnu-libstdc++-headers-$VERSION.tar.bz2"
        dump "Packaging: $PACKAGE"
        pack_archive "$PACKAGE" "$NDK_DIR" "$GNUSTL_SUBDIR/$VERSION/include"

        # Then, one package per version/ABI for libraries
        for ABI in $ABIS; do
            FILES=""
            for LIB in include/bits libsupc++.a libgnustl_static.a libgnustl_shared.so; do
                FILES="$FILES $GNUSTL_SUBDIR/$VERSION/libs/$ABI/$LIB"
                THUMB_FILE="$GNUSTL_SUBDIR/$VERSION/libs/$ABI/thumb/$LIB"
                if [ -f "$NDK_DIR/$THUMB_FILE" ] ; then
                    FILES="$FILES $THUMB_FILE"
                fi
            done
            PACKAGE="$PACKAGE_DIR/gnu-libstdc++-libs-$VERSION-$ABI.tar.bz2"
            dump "Packaging: $PACKAGE"
            pack_archive "$PACKAGE" "$NDK_DIR" "$FILES"
            fail_panic "Could not package $ABI STLport binaries!"
        done
    done
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    log "Cleaning up..."
    rm -rf $BUILD_DIR
else
    log "Don't forget to cleanup: $BUILD_DIR"
fi

log "Done!"
