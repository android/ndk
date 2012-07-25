#!/bin/sh
#
# Copyright (C) 2012 The Android Open Source Project
# Copyright (C) 2012 Ray Donnelly <mingw.android at gmail.com>
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
# Rebuild the host Python binaries from sources.
#

# include common function and variable definitions
NDK_BUILDTOOLS_PATH="$(dirname $0)"
. "$NDK_BUILDTOOLS_PATH/prebuilt-common.sh"
. "$NDK_BUILDTOOLS_PATH/common-build-host-funcs.sh"

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION="\
This program is used to rebuild one or more Python client programs from
sources. To use it, you will need a working set of toolchain sources, like
those downloaded with download-toolchain-sources.sh, then pass the
corresponding directory with the --toolchain-src-dir=<path> option.

By default, the script rebuilds Python for you host system [$HOST_TAG],
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

For example, here's how to rebuild Python 2.7.3 on Linux
for six different systems:

  $PROGNAME --build-dir=/path/to/toolchain/src \
    --python-version=2.7.3 \
    --systems=linux-x86,linux-x86_64,windows,windows-x86_64,darwin-x86,darwin-x86_64"

TOOLCHAIN_SRC_DIR=
register_var_option "--toolchain-src-dir=<path>" TOOLCHAIN_SRC_DIR "Select toolchain source directory"

PYTHON_VERSION=2.7.3
register_var_option "--python-version=<version>" PYTHON_VERSION "Select Python version."

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Select NDK install directory."

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package prebuilt tarballs into directory."

BUILD_DIR=
register_var_option "--build-dir=<path>" BUILD_DIR "Build Python into directory"

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

# Python needs to execute itself during its build process, so must build the build
# Python first. It should also be an error if not asked to build for build machine.
BH_HOST_SYSTEMS=$(bh_sort_systems_build_first "$BH_HOST_SYSTEMS")

# Make sure that the the user asked for the build OS's Python to be built.
#  and that the above sort command worked correctly.
if [ ! "$(bh_list_contains $BH_BUILD_TAG $BH_HOST_SYSTEMS)" = "first" ] ; then
    panic "Cross building Python requires building for the build OS!"
fi

download_package ()
{
    # Assume the packages are already downloaded under $ARCHIVE_DIR
    local PKG_URL=$1
    local PKG_NAME=$(basename $PKG_URL)

    case $PKG_NAME in
        *.tar.bz2)
            PKG_BASENAME=${PKG_NAME%%.tar.bz2}
            ;;
        *.tar.gz)
            PKG_BASENAME=${PKG_NAME%%.tar.gz}
            ;;
        *)
            panic "Unknown archive type: $PKG_NAME"
    esac

    if [ ! -f "$ARCHIVE_DIR/$PKG_NAME" ]; then
        log "Downloading $PKG_URL..."
        (cd $ARCHIVE_DIR && run curl -L -o "$PKG_NAME" "$PKG_URL")
        fail_panic "Can't download '$PKG_URL'"
    fi

    if [ ! -d "$SRC_DIR/$PKG_BASENAME" ]; then
        log "Uncompressing $PKG_URL into $SRC_DIR"
        case $PKG_NAME in
            *.tar.bz2)
                run tar xjf $ARCHIVE_DIR/$PKG_NAME -C $SRC_DIR
                ;;
            *.tar.gz)
                run tar xzf $ARCHIVE_DIR/$PKG_NAME -C $SRC_DIR
                ;;
            *)
                panic "Unknown archive type: $PKG_NAME"
                ;;
        esac
        fail_panic "Can't uncompress $ARCHIVE_DIR/$PKG_NAME"
    fi
}

if [ -z "$BUILD_DIR" ] ; then
    BUILD_DIR=/tmp/ndk-$USER/buildhost
fi

bh_setup_build_dir $BUILD_DIR

if [ "$BH_BUILD_MODE" = "debug" ] ; then
   PYDEBUG="-with-pydebug"
fi

# Sanity check that we have the right compilers for all hosts
for SYSTEM in $BH_HOST_SYSTEMS; do
    bh_setup_build_for_host $SYSTEM
done

TEMP_DIR=$BUILD_DIR/tmp
# Download and unpack source packages from official sites
ARCHIVE_DIR=$TEMP_DIR/archive
STAMP_DIR=$TEMP_DIR/timestamps
BUILD_DIR=$TEMP_DIR/build-$HOST_TAG

mkdir -p $BUILD_DIR

PROGDIR=`dirname $0`
PROGDIR=$(cd $PROGDIR && pwd)

# Sanity check for all Python versions.
for VERSION in $(commas_to_spaces $PYTHON_VERSION); do
    PYTHON_SRCDIR=$TOOLCHAIN_SRC_DIR/python/Python-$VERSION
    if [ ! -d "$PYTHON_SRCDIR" ]; then
        panic "Missing source directory: $PYTHON_SRCDIR"
    fi
done

# Return the build install directory of a given Python version
# $1: host system tag
# $2: python version
# The suffix of this has to match python_ndk_install_dir
#  as I package them from the build folder, substituting
#  the end part of python_build_install_dir matching python_ndk_install_dir
#  with nothing.
# Needs to match with
#  python_build_install_dir () in build-host-gdb.sh
python_build_install_dir ()
{
    echo "$BH_BUILD_DIR/install/prebuilt/$1"
}

# $1: host system tag
# $2: python version
python_ndk_package_name ()
{
    echo "python-$2"
}


# Same as python_build_install_dir, but for the final NDK installation
# directory. Relative to $NDK_DIR.
python_ndk_install_dir ()
{
    echo "prebuilt/$1"
}

arch_to_qemu_arch ()
{
    case $1 in
        x86)
            echo i386
            ;;
        *)
            echo $1
            ;;
    esac
}

# $1: host system tag
# $2: python version
build_host_python ()
{
    local SRCDIR=$TOOLCHAIN_SRC_DIR/python/Python-$2
    local BUILDDIR=$BH_BUILD_DIR/build-python-$1-$2
    local INSTALLDIR=$(python_build_install_dir $1 $2)
    local ARGS TEXT

    if [ ! -f "$SRCDIR/configure" ]; then
        panic "Missing configure script in $SRCDIR"
    fi

    ARGS=" --prefix=$INSTALLDIR"

    # Python considers it cross compiling if --host is passed
    #  and that then requires that a CONFIG_SITE file is used.
    # This is not necessary if it's only the arch that differs.
    if [ ! $BH_HOST_CONFIG = $BH_BUILD_CONFIG ] ; then
        ARGS=$ARGS" --build=$BH_BUILD_CONFIG"
    fi
    ARGS=$ARGS" --host=$BH_HOST_CONFIG"
    ARGS=$ARGS" $PYDEBUG"
    ARGS=$ARGS" --disable-ipv6"

    mkdir -p "$BUILDDIR" && rm -rf "$BUILDDIR"/*
    cd "$BUILDDIR" &&
    bh_setup_host_env

    CFG_SITE=
    # Need to add -L$HOST_STATIC_LIBDIR to LDSHARED if need
    # any static host libs.
    export LDSHARED="$CC -shared "
    if [ ! $BH_HOST_TAG = $BH_BUILD_TAG ]; then

        # Cross compiling.
        CFG_SITE=$BUILDDIR/config.site

        # Ideally would remove all of these configury hacks by
        # patching the issues.

        if [ $1 = darwin-x86 -o $1 = darwin-x86_64 ]; then
            echo "ac_cv_file__dev_ptmx=no"              > $CFG_SITE
            echo "ac_cv_file__dev_ptc=no"              >> $CFG_SITE
            echo "ac_cv_have_long_long_format=yes"     >> $CFG_SITE
            if [ $1 = darwin-x86 ] ; then
                echo "ac_osx_32bit=yes"                >> $CFG_SITE
            elif [ $1 = darwin-x86_64 ] ; then
                echo "ac_osx_32bit=no"                 >> $CFG_SITE
            fi
            echo "ac_cv_have_sendfile=no"              >> $CFG_SITE
            # I could change AC_MSG_CHECKING(LDSHARED) in configure.ac
            # to check $host instead of $ac_sys_system/$ac_sys_release
            # but it handles loads of platforms
            # and I can only test on three, so instead...
            export LDSHARED="$CC -bundle -undefined dynamic_lookup"
        elif [ $1 = windows-x86 -o $1 = windows-x86_64 ]; then
            echo "ac_cv_file__dev_ptmx=no"              > $CFG_SITE
            echo "ac_cv_file__dev_ptc=no"              >> $CFG_SITE
            CFLAGS=$CFLAGS" -D__USE_MINGW_ANSI_STDIO=1"
            CXXFLAGS=$CXXFLAGS" -D__USE_MINGW_ANSI_STDIO=1"
        elif [ $1 = linux-x86 -o $1 = linux-x86_64 ]; then
            echo "ac_cv_file__dev_ptmx=yes"             > $CFG_SITE
            echo "ac_cv_file__dev_ptc=no"              >> $CFG_SITE
            echo "ac_cv_have_long_long_format=yes"     >> $CFG_SITE
            echo "ac_cv_pthread_system_supported=yes"  >> $CFG_SITE
            echo "ac_cv_working_tzset=yes"             >> $CFG_SITE
            echo "ac_cv_little_endian_double=yes"      >> $CFG_SITE
        fi

        if [ $BH_HOST_OS = $BH_BUILD_OS ]; then
            # Only cross compiling from arch perspective.
            # qemu causes failures as cross-compilation is not detected
            # if a test executable can be run successfully, so we test
            # for qemu-${BH_HOST_ARCH} and qemu-${BH_HOST_ARCH}-static
            # and panic if either are found.
            QEMU_HOST_ARCH=$(arch_to_qemu_arch $BH_HOST_ARCH)
            if [ ! -z "$(which qemu-$QEMU_HOST_ARCH 2>/dev/null)" -o \
                 ! -z "$(which qemu-$QEMU_HOST_ARCH-static 2>/dev/null)" ] ; then
               panic "Installed qemu(s) ($(which qemu-$QEMU_HOST_ARCH 2>/dev/null) $(which qemu-$QEMU_HOST_ARCH-static 2>/dev/null))" \
                      "will prevent this build from working."
            fi
        fi
    fi

    TEXT="$(bh_host_text) python-$BH_HOST_CONFIG:"

    touch $SRCDIR/Include/graminit.h
    touch $SRCDIR/Python/graminit.c
    echo "" > $SRCDIR/Parser/pgen.stamp

    dump "$TEXT Building"
    export CONFIG_SITE=$CFG_SITE &&
    run2 "$SRCDIR"/configure $ARGS &&
    run2 make -j$NUM_JOBS &&
    run2 make -j$NUM_JOBS install
}

need_build_host_python ()
{
    bh_stamps_do host-python-$1-$2 build_host_python $1 $2
}

# Install host Python binaries and support files to the NDK install dir.
# $1: host tag
# $2: python version
install_host_python ()
{
    local SRCDIR="$(python_build_install_dir $1 $2)"
    local DSTDIR="$NDK_DIR/$(python_ndk_install_dir $1 $2)"

    need_build_host_python $1 $2

    dump "$(bh_host_text) python-$BH_HOST_ARCH-$2: Installing"
    run copy_directory "$SRCDIR/bin"     "$DSTDIR/bin"
    run copy_directory "$SRCDIR/lib"     "$DSTDIR/lib"
    run copy_directory "$SRCDIR/share"   "$DSTDIR/share"
    run copy_directory "$SRCDIR/include" "$DSTDIR/include"
}

need_install_host_python ()
{
    local SRCDIR="$(python_build_install_dir $1 $2)"

    bh_stamps_do install-host-python-$1-$2 install_host_python $1 $2

    # make sharedmods (setup.py) needs to use the build machine's Python
    # for the other hosts to build correctly.
    if [ $BH_BUILD_TAG = $BH_HOST_TAG ]; then
        export PATH=$SRCDIR/bin:$PATH
    fi
}

# Package host Python binaries into a tarball
# $1: host tag
# $2: python version
package_host_python ()
{
    local BLDDIR="$(python_build_install_dir $1 $2)"
    local SRCDIR="$(python_ndk_install_dir $1 $2)"
    # This is similar to BLDDIR=${BLDDIR%%$SRCDIR}
    BLDDIR=$(echo "$BLDDIR" | sed "s/$(echo "$SRCDIR" | sed -e 's/\\/\\\\/g' -e 's/\//\\\//g' -e 's/&/\\\&/g')//g")
    local PACKAGENAME=$(python_ndk_package_name $1 $2)-$1.tar.bz2
    local PACKAGE="$PACKAGE_DIR/$PACKAGENAME"

    need_install_host_python $1 $2

    dump "$(bh_host_text) $PACKAGENAME: Packaging"
    run pack_archive "$PACKAGE" "$BLDDIR" "$SRCDIR"
}

PYTHON_VERSION=$(commas_to_spaces $PYTHON_VERSION)
ARCHS=$(commas_to_spaces $ARCHS)

# Let's build this
for SYSTEM in $BH_HOST_SYSTEMS; do
    bh_setup_build_for_host $SYSTEM
    for VERSION in $PYTHON_VERSION; do
        need_install_host_python $SYSTEM $VERSION
    done
done

if [ "$PACKAGE_DIR" ]; then
    for SYSTEM in $BH_HOST_SYSTEMS; do
        bh_setup_build_for_host $SYSTEM
        for VERSION in $PYTHON_VERSION; do
            package_host_python $SYSTEM $VERSION
        done
    done
fi
