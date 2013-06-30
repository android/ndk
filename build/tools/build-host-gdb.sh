#!/bin/sh
#
# Copyright (C) 2012 The Android Open Source Project
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
# Rebuild the host GDB binaries from sources.
#

# include common function and variable definitions
NDK_BUILDTOOLS_PATH="$(dirname $0)"
. "$NDK_BUILDTOOLS_PATH/prebuilt-common.sh"
. "$NDK_BUILDTOOLS_PATH/common-build-host-funcs.sh"

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION="\
This program is used to rebuild one or more NDK gdb client programs from
sources. To use it, you will need a working set of toolchain sources, like
those downloaded with download-toolchain-sources.sh, then pass the
corresponding directory with the --toolchain-src-dir=<path> option.

By default, the script rebuilds GDB for you host system [$HOST_TAG],
but you can use --systems=<tag1>,<tag2>,.. to ask binaries that can run on
several distinct systems. Each <tag> value in the list can be one of the
following:

   linux-x86
   linux-x86_64
   windows
   windows-x86  (equivalent to 'windows')
   windows-x86_64
   darwin-x86
   darwin-x86_64

For example, here's how to rebuild the ARM toolchains on Linux
for four different systems:

  $PROGNAME --toolchain-src-dir=/path/to/toolchain/src \
    --systems=linux-x86,linux-x86_64,windows,windows-x86_64 \
    --arch=arm"

TOOLCHAIN_SRC_DIR=
register_var_option "--toolchain-src-dir=<path>" TOOLCHAIN_SRC_DIR "Select toolchain source directory"

GDB_VERSION="6.6 7.3.x"
register_var_option "--gdb-version=<version>" GDB_VERSION "Select GDB version(s)."

BUILD_DIR=
register_var_option "--build-dir=<path>" BUILD_DIR "Build GDB into directory"

PYTHON_VERSION=
register_var_option "--python-version=<version>" PYTHON_VERSION "Python version."

PYTHON_BUILD_DIR=
register_var_option "--python-build-dir=<path>" PYTHON_BUILD_DIR "Python build directory."

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Select NDK install directory."

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package prebuilt tarballs into directory."

ARCHS=$DEFAULT_ARCHS
register_var_option "--arch=<list>" ARCHS "Build GDB client for these CPU architectures."

bh_register_options

register_jobs_option

extract_parameters "$@"

if [ -n "$PARAMETERS" ]; then
    panic "This script doesn't take parameters, only options. See --help"
fi

if [ -z "$TOOLCHAIN_SRC_DIR" ]; then
    panic "Please use --toolchain-src-dir=<path> to select toolchain source directory."
fi

BH_HOST_SYSTEMS=$(commas_to_spaces $BH_HOST_SYSTEMS)

# Sanity check for all GDB versions
for VERSION in $(commas_to_spaces $GDB_VERSION); do
    GDB_SRCDIR=$TOOLCHAIN_SRC_DIR/gdb/gdb-$VERSION
    if [ ! -d "$GDB_SRCDIR" ]; then
        panic "Missing source directory: $GDB_SRCDIR"
    fi
done

if [ -z "$BUILD_DIR" ] ; then
    BUILD_DIR=/tmp/ndk-$USER/buildgdb
fi

bh_setup_build_dir $BUILD_DIR

# Sanity check that we have the right compilers for all hosts
for SYSTEM in $BH_HOST_SYSTEMS; do
    bh_setup_build_for_host $SYSTEM
done

# Return the build install directory of a given GDB version
# $1: host system tag
# $2: target system tag
# $3: gdb version
gdb_build_install_dir ()
{
    echo "$BH_BUILD_DIR/install/$1/gdb-$(get_toolchain_name_for_arch $(bh_tag_to_arch $2))-$3"
}

# $1: host system tag
# $2: target system tag
# $3: gdb version
gdb_ndk_package_name ()
{
    echo "gdb-$(get_toolchain_name_for_arch $(bh_tag_to_arch $2))-$3-$1"
}


# Same as gdb_build_install_dir, but for the final NDK installation
# directory. Relative to $NDK_DIR.
gdb_ndk_install_dir ()
{
    echo "toolchains/gdb-$(get_toolchain_name_for_arch $(bh_tag_to_arch $2))-$3/prebuilt/$(install_dir_from_host_tag $1)"
}

python_build_install_dir ()
{
    echo "$PYTHON_BUILD_DIR/install/prebuilt/$(install_dir_from_host_tag $1)"
}

# $1: host system tag
build_expat ()
{
    local ARGS
    local SRCDIR=$TOOLCHAIN_SRC_DIR/expat/expat-2.0.1
    local BUILDDIR=$BH_BUILD_DIR/build-expat-2.0.1-$1
    local INSTALLDIR=$BH_BUILD_DIR/install-host-$1

    ARGS=" --prefix=$INSTALLDIR"
    ARGS=$ARGS" --disable-shared --enable-static"
    ARGS=$ARGS" --build=$BH_BUILD_CONFIG"
    ARGS=$ARGS" --host=$BH_HOST_CONFIG"

    TEXT="$(bh_host_text) expat:"

    mkdir -p "$BUILDDIR" && rm -rf "$BUILDDIR"/* &&
    cd "$BUILDDIR" &&
    dump "$TEXT Building"
    run2 "$SRCDIR"/configure $ARGS &&
    run2 make -j$NUM_JOBS &&
    run2 make -j$NUM_JOBS install
}

need_build_expat ()
{
    bh_stamps_do host-expat-$1 build_expat $1
}

# $1: host system tag
# $2: target tag
# $3: gdb version
build_host_gdb ()
{
    local SRCDIR=$TOOLCHAIN_SRC_DIR/gdb/gdb-$3
    local BUILDDIR=$BH_BUILD_DIR/build-gdb-$1-$2-$3
    local INSTALLDIR=$(gdb_build_install_dir $1 $2 $3)
    local ARGS TEXT

    if [ ! -f "$SRCDIR/configure" ]; then
        panic "Missing configure script in $SRCDIR"
    fi

    bh_set_target_tag $2
    bh_setup_host_env

    need_build_expat $1
    local EXPATPREFIX=$BH_BUILD_DIR/install-host-$1

    ARGS=" --prefix=$INSTALLDIR"
    ARGS=$ARGS" --disable-shared"
    ARGS=$ARGS" --build=$BH_BUILD_CONFIG"
    ARGS=$ARGS" --host=$BH_HOST_CONFIG"
    ARGS=$ARGS" --target=$(bh_tag_to_config_triplet $2)"
    ARGS=$ARGS" --disable-werror"
    ARGS=$ARGS" --disable-nls"
    ARGS=$ARGS" --disable-docs"
    ARGS=$ARGS" --with-expat"
    ARGS=$ARGS" --with-libexpat-prefix=$EXPATPREFIX"
    if [ -n "$PYTHON_VERSION" ]; then
        ARGS=$ARGS" --with-python=$(python_build_install_dir $BH_HOST_TAG)/bin/python-config.sh"
        if [ $1 = windows-x86 -o $1 = windows-x86_64 ]; then
            # This is necessary for the Python integration to build.
            CFLAGS=$CFLAGS" -D__USE_MINGW_ANSI_STDIO=1"
            CXXFLAGS=$CXXFLAGS" -D__USE_MINGW_ANSI_STDIO=1"
        fi
    fi
    TEXT="$(bh_host_text) gdb-$BH_TARGET_ARCH-$3:"

    mkdir -p "$BUILDDIR" && rm -rf "$BUILDDIR"/* &&
    cd "$BUILDDIR" &&
    dump "$TEXT Building"
    run2 "$SRCDIR"/configure $ARGS &&
    run2 make -j$NUM_JOBS &&
    run2 make -j$NUM_JOBS install
    fail_panic "Failed to configure/make/install gdb"
}

need_build_host_gdb ()
{
    bh_stamps_do host-gdb-$1-$2-$3 build_host_gdb $1 $2 $3
}

# Install host GDB binaries and support files to the NDK install dir.
# $1: host tag
# $2: target tag
# $3: gdb version
install_host_gdb ()
{
    local SRCDIR="$(gdb_build_install_dir $1 $2 $3)"
    local DSTDIR="$NDK_DIR/$(gdb_ndk_install_dir $1 $2 $3)"
    local PYDIR="$NDK_DIR/$(python_ndk_install_dir $1)"

    need_build_host_gdb $1 $2 $3

    bh_set_target_tag $2

    dump "$(bh_host_text) gdb-$BH_TARGET_ARCH-$3: Installing"
    run copy_directory "$SRCDIR/bin" "$DSTDIR/bin"
    if [ -d "$SRCDIR/share/gdb" ]; then
        run copy_directory "$SRCDIR/share/gdb" "$DSTDIR/share/gdb"
    fi

    # build the gdb stub and replace gdb with it. This is done post-install
    # so files are in the correct place when determining the relative path.

    case "$1" in
        windows*)
            dump "$TEXT Building gdb-stub"
            bh_setup_host_env
            GCC_FOR_STUB=${BH_HOST_CONFIG}-gcc
            GCC_FOR_STUB_TARGET=`$GCC_FOR_STUB -dumpmachine`
            if [ "$GCC_FOR_STUB_TARGET" = "i586-mingw32msvc" ]; then
                GCC_FOR_STUB=i686-w64-mingw32-gcc
                dump "Override compiler for gdb-stub: $GCC_FOR_STUB"
            fi

            # Uses $TOOLCHAIN_PATH/bin/$(bh_tag_to_config_triplet $2)-gdb.exe (1) instead of
            # ${DSTDIR}/bin/$(bh_tag_to_config_triplet $2)-gdb.exe (2) because
            # the final layout is to (1) which is a folder deeper than (2).
            # Sample (1):
            #  $NDK/gdb-arm-linux-androideabi-7.6/prebuilt/windows/bin/arm-linux-androideabi-gdb.exe
            # Sample (2):
            #  $NDK/toolchains/arm-linux-androideabi-4.7/prebuilt/windows/bin/arm-linux-androideabi-gdb.exe
            run $NDK_BUILDTOOLS_PATH/build-gdb-stub.sh \
                --gdb-executable-path=${DSTDIR}/bin/$(bh_tag_to_config_triplet $2)-gdb.exe \
                --python-prefix-dir=${PYDIR} \
                --mingw-w64-gcc=${GCC_FOR_STUB}
            ;;
        *)
            ;;
    esac
}

need_install_host_gdb ()
{
    bh_stamps_do install-host-gdb-$1-$2-$3 install_host_gdb $1 $2 $3
}

# Package host GDB binaries into a tarball
# $1: host tag
# $2: target tag
# $3: gdb version
package_host_gdb ()
{
    local SRCDIR="$(gdb_ndk_install_dir $1 $2 $3)"
    local PACKAGENAME=$(gdb_ndk_package_name $1 $2 $3).tar.bz2
    local PACKAGE="$PACKAGE_DIR/$PACKAGENAME"

    need_install_host_gdb $1 $2 $3

    bh_set_target_tag $2

    dump "$(bh_host_text) $PACKAGENAME: Packaging"
    run pack_archive "$PACKAGE" "$NDK_DIR" "$SRCDIR"
}

GDB_VERSION=$(commas_to_spaces $GDB_VERSION)
ARCHS=$(commas_to_spaces $ARCHS)

# Let's build this
for SYSTEM in $BH_HOST_SYSTEMS; do
    bh_setup_build_for_host $SYSTEM
    for ARCH in $ARCHS; do
        for VERSION in $GDB_VERSION; do
            need_install_host_gdb $SYSTEM android-$ARCH $VERSION
        done
    done
done

if [ "$PACKAGE_DIR" ]; then
    for SYSTEM in $BH_HOST_SYSTEMS; do
        bh_setup_build_for_host $SYSTEM
        for ARCH in $ARCHS; do
            for VERSION in $GDB_VERSION; do
                bh_stamps_do package_host_gdb-$SYSTEM-$ARCH-$VERSION package_host_gdb $SYSTEM android-$ARCH $VERSION
            done
        done
    done
fi
