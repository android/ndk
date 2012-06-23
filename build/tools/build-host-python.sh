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

For example, here's how to rebuild the ARM toolchains on Linux
for four different systems:

  $PROGNAME --toolchain-src-dir=/path/to/toolchain/src \
    --systems=linux-x86,linux-x86_64,windows,windows-x86_64"

TOOLCHAIN_SRC_DIR=
register_var_option "--toolchain-src-dir=<path>" TOOLCHAIN_SRC_DIR "Select toolchain source directory."

PYTHON_VERSION=3.3.0a2
register_var_option "--python-version=<version>" PYTHON_VERSION "Select Python version."

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Select NDK install directory."

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package prebuilt tarballs into directory."

bh_register_options

register_jobs_option

extract_parameters "$@"

if [ $_BH_OPTION_NO_STRIP = yes ] ; then
   PYDEBUG="-with-pydebug"
fi

if [ -n "$PARAMETERS" ]; then
    panic "This script doesn't take parameters, only options. See --help"
fi

if [ -z "$TOOLCHAIN_SRC_DIR" ]; then
    panic "Please use --toolchain-src-dir=<path> to select toolchain source directory."
fi

BH_HOST_SYSTEMS=$(commas_to_spaces $BH_HOST_SYSTEMS)

# Python needs to execute itself during its build process, so myst build the build
# Python first. We should also fatal out if not asked to build for build machine.
BH_HOST_SYSTEMS=$(bh_sort_systems_build_first "$BH_HOST_SYSTEMS")

# Sanity check for all Python versions
for VERSION in $(commas_to_spaces $PYTHON_VERSION); do
    PYTHON_SRCDIR=$TOOLCHAIN_SRC_DIR/python/python-$VERSION
    if [ ! -d "$PYTHON_SRCDIR" ]; then
        panic "Missing source directory: $PYTHON_SRCDIR"
    fi
done

bh_setup_build_dir /tmp/ndk-$USER/buildpythonhost

# Sanity check that we have the right compilers for all hosts
for SYSTEM in $BH_HOST_SYSTEMS; do
    bh_setup_build_for_host $SYSTEM
done

# Return the build install directory of a given Python version
# $1: host system tag
# $2: python version
python_build_install_dir ()
{
    echo "$BH_BUILD_DIR/install/$1/python-$2"
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
    echo "prebuilt/$1/python-$2"
}

# $1: host system tag
# $2: python version
build_host_python ()
{
    local SRCDIR=$TOOLCHAIN_SRC_DIR/python/python-$2
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
    if [ ! $BH_HOST_OS = $BH_BUILD_OS ] ; then
        ARGS=$ARGS" --build=$BH_BUILD_CONFIG"
    fi
    ARGS=$ARGS" --host=$BH_HOST_CONFIG"
# This config argument causes a segmentation fault.
#    ARGS=$ARGS" --without-doc-strings"
    ARGS=$ARGS" $PYDEBUG"

    mkdir -p "$BUILDDIR" && rm -rf "$BUILDDIR"/*
    cd "$BUILDDIR" &&
    bh_setup_host_env

    CFG_SITE=
    # Need to add -L$HOST_STATIC_LIBDIR to LDSHARED if need
    # any static host libs.
    export LDSHARED="$CC -shared "
    if [ $1 = darwin-x86 -o $1 = darwin-x86_64 ]; then
        CFG_SITE=$BUILDDIR/config.site
        echo "ac_cv_file__dev_ptmx=no"          > $CFG_SITE
        echo "ac_cv_file__dev_ptc=no"          >> $CFG_SITE
        echo "ac_cv_have_long_long_format=yes" >> $CFG_SITE
        if [ $1 = darwin-x86 ] ; then
            echo "ac_osx_32bit=yes"                >> $CFG_SITE
        elif [ $1 = darwin-x86_64 ] ; then
            echo "ac_osx_32bit=no"                 >> $CFG_SITE
        fi
        echo "ac_cv_have_sendfile=no"          >> $CFG_SITE
	# I could change AC_MSG_CHECKING(LDSHARED) in configure.ac
	# to check $host instead of $ac_sys_system/$ac_sys_release
	# but it handles loads of platforms
	# and I can only test on three, so instead...
        export LDSHARED="$CC -undefined dynamic_lookup "
    fi

    TEXT="$(bh_host_text) python-$BH_HOST_CONFIG:"

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
    run copy_directory "$SRCDIR/bin" "$DSTDIR/bin"
    if [ -d "$SRCDIR/share/python" ]; then
        run copy_directory "$SRCDIR/share/python" "$DSTDIR/share/python"
    fi
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
    local SRCDIR="$(python_ndk_install_dir $1 $2)"
    local PACKAGENAME=$(python_ndk_package_name $1 $2)-$1.tar.bz2
    local PACKAGE="$PACKAGE_DIR/$PACKAGENAME"

    need_install_host_python $1 $2

    dump "$(bh_host_text) $PACKAGENAME: Packaging"
    run pack_archive "$PACKAGE" "$NDK_DIR" "$SRCDIR"
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
