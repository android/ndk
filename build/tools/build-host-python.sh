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
# Also copies any gnu libstdc++ pretty-printers
# found in $TOOLCHAIN_SRC_DIR/gcc/gcc-*
# and Joachim Reichel's stlport pretty printers
# found in sources/host-tools/gdb-pretty-printers/stlport
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

For example, here's how to rebuild Python 2.7.5 on Linux
for six different systems:

  $PROGNAME --build-dir=/path/to/toolchain/src \n \
    --python-version=2.7.5 \n \
    --systems=linux-x86,linux-x86_64,windows,windows-x86_64,darwin-x86,darwin-x86_64"

TOOLCHAIN_SRC_DIR=
register_var_option "--toolchain-src-dir=<path>" TOOLCHAIN_SRC_DIR "Select toolchain source directory"

PYTHON_VERSION=$DEFAULT_PYTHON_VERSION
register_var_option "--python-version=<version>" PYTHON_VERSION "Select Python version."

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Select NDK install directory."

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package prebuilt tarballs into directory."

BUILD_DIR=
register_var_option "--build-dir=<path>" BUILD_DIR "Build Python into directory"

bh_register_options
register_try64_option
register_canadian_option
register_jobs_option

extract_parameters "$@"

if [ -n "$PARAMETERS" ]; then
    panic "This script doesn't take parameters, only options. See --help"
fi

if [ -z "$TOOLCHAIN_SRC_DIR" ]; then
    panic "Please use --toolchain-src-dir=<path> to select toolchain source directory."
fi
check_toolchain_src_dir "$TOOLCHAIN_SRC_DIR"
TOOLCHAIN_SRC_DIR=`cd $TOOLCHAIN_SRC_DIR; pwd`

BH_HOST_SYSTEMS=$(commas_to_spaces $BH_HOST_SYSTEMS)
AUTO_BUILD="no"

if [ "$MINGW" = "yes" ]; then
    BH_HOST_SYSTEMS="windows"
    log "Auto-config: --systems=windows"
fi

if [ "$DARWIN" = "yes" ]; then
    BH_HOST_SYSTEMS="darwin-x86"
    log "Auto-config: --systems=darwin-x86"
fi

determine_systems ()
{
    local IN_SYSTEMS="$1"
    local OUT_SYSTEMS

    for SYSTEM in $IN_SYSTEMS; do
        if [ "$TRY64" = "yes" ]; then
            case $SYSTEM in
                darwin-x86|linux-x86|windows-x86)
                    SYSTEM=${SYSTEM%%x86}x86_64
                    ;;
                windows)
                    SYSTEM=windows-x86_64
                    ;;
            esac
        else
            # 'windows-x86' causes substitution
            # failure at the packing stage.
            case $SYSTEM in
                windows-x86)
                    SYSTEM=windows
                    ;;
            esac
        fi
        OUT_SYSTEMS="$OUT_SYSTEMS $SYSTEM"
    done
    echo $OUT_SYSTEMS
}

BH_HOST_SYSTEMS=$(determine_systems "$BH_HOST_SYSTEMS")

# Build python for build machine automatically
if [ "$(bh_list_contains $BH_BUILD_TAG $BH_HOST_SYSTEMS)" = "no" ]; then
    BH_HOST_SYSTEMS="$BH_BUILD_TAG $BH_HOST_SYSTEMS"
    AUTO_BUILD="yes"
fi

# Python needs to execute itself during its build process, so must build the build
# Python first. It should also be an error if not asked to build for build machine.
BH_HOST_SYSTEMS=$(bh_sort_systems_build_first "$BH_HOST_SYSTEMS")

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

    # Currently, 2.7.5 and 3.3.0 builds generate $SRCDIR/Lib/_sysconfigdata.py, unless it
    # already exists (in which case it ends up wrong anyway!)... this should really be in
    # the build directory instead.
    if [ ! -f "$SRCDIR/Lib/_sysconfigdata.py" ]; then
        log "Removing old $SRCDIR/Lib/_sysconfigdata.py"
        rm -f $SRCDIR/Lib/_sysconfigdata.py
    fi

    ARGS=" --prefix=$INSTALLDIR"

    ARGS=$ARGS" --build=$BH_BUILD_CONFIG"
    ARGS=$ARGS" --host=$BH_HOST_CONFIG"
    ARGS=$ARGS" --with-build-sysroot"
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

        if [ "$BH_HOST_OS" = "$BH_BUILD_OS" ]; then
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
    else
        if [ $1 = darwin-x86 -o $1 = darwin-x86_64 ]; then
            export LDSHARED="$CC -bundle -undefined dynamic_lookup"
        fi
    fi

    TEXT="$(bh_host_text) python-$BH_HOST_CONFIG-$2:"

    touch $SRCDIR/Include/graminit.h
    touch $SRCDIR/Python/graminit.c
    echo "" > $SRCDIR/Parser/pgen.stamp
    touch $SRCDIR/Parser/Python.asdl
    touch $SRCDIR/Parser/asdl.py
    touch $SRCDIR/Parser/asdl_c.py
    touch $SRCDIR/Include/Python-ast.h
    touch $SRCDIR/Python/Python-ast.c

    # By default, the Python build will force the following compiler flags
    # after our own CFLAGS:
    #   -g -fwrap -O3 -Wall -Wstrict-prototypes
    #
    # The '-g' is unfortunate because it makes the generated binaries
    # much larger than necessary, and stripping them after the fact is
    # a bit delicate when cross-compiling. To avoid this, define a
    # custom OPT variable here (see Python-2.7.5/configure.ac) when
    # generating non stripped builds.
    if [ "$BH_BUILD_MODE" = "release" ]; then
      OPT="-fwrapv -O3 -Wall -Wstrict-prototypes"
      export OPT
    fi

    dump "$TEXT Building"
    export CONFIG_SITE=$CFG_SITE &&
    run2 "$SRCDIR"/configure $ARGS &&
    #
    # Note 1:
    # sharedmods is a phony target, but it's a dependency of both "make all" and also
    # "make install", this causes it to fail on Windows as it tries to rename pydoc3
    # to pydoc3.3 twice, and the second time aroud the file exists. So instead, we
    # just do make install.
    #
    # Note 2:
    # Can't run make install with -j as from the Makefile:
    # install:	 altinstall bininstall maninstall
    #  meaning altinstall and bininstall are kicked off at the same time
    #  but actually, bininstall depends on altinstall being run first
    #  due to libainstall: doing
    #  $(INSTALL_SCRIPT) python-config $(DESTDIR)$(BINDIR)/python$(VERSION)-config
    #  and bininstall: doing
    #  (cd $(DESTDIR)$(BINDIR); $(LN) -s python$(VERSION)-config python2-config)
    #  Though the real fix is to make bininstall depend on libainstall.
    run2 make -j$NUM_JOBS &&
    run2 make install

    # Pretty printers.
    PYPPDIR="$INSTALLDIR/share/pretty-printers/"

    # .. for gnu stdlibc++
    GCC_DIRS=$(find $TOOLCHAIN_SRC_DIR/gcc/ -maxdepth 1 -name "gcc-*" -type d)
    for GCC_DIR in $GCC_DIRS; do
        (
        if [ -d "$GCC_DIR/libstdc++-v3/python" ]; then
            cd "$GCC_DIR/libstdc++-v3/python"
            [ -d "$PYPPDIR/libstdcxx/$(basename $GCC_DIR)" ] || mkdir -p "$PYPPDIR/libstdcxx/$(basename $GCC_DIR)"
            run2 find . -path "*.py" -exec cp {} "$PYPPDIR/libstdcxx/$(basename $GCC_DIR)/" \;
        fi
        )
    done

    # .. for STLPort
    run2 cp -rf $NDK_DIR/sources/host-tools/gdb-pretty-printers/stlport/gppfs-0.2 $PYPPDIR/stlport
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

    if [ $AUTO_BUILD != "yes" -o $1 != $BH_BUILD_TAG ]; then
        dump "$(bh_host_text) python-$BH_HOST_ARCH-$2: Installing"
        run copy_directory "$SRCDIR/bin"     "$DSTDIR/bin"
        run copy_directory "$SRCDIR/lib"     "$DSTDIR/lib"
        run copy_directory "$SRCDIR/share"   "$DSTDIR/share"
        run copy_directory "$SRCDIR/include" "$DSTDIR/include"
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
    local BLDDIR="$(python_build_install_dir $1 $2)"
    local SRCDIR="$(python_ndk_install_dir $1 $2)"
    # This is similar to BLDDIR=${BLDDIR%%$SRCDIR} (and requires we use windows and not windows-x86)
    BLDDIR=$(echo "$BLDDIR" | sed "s/$(echo "$SRCDIR" | sed -e 's/\\/\\\\/g' -e 's/\//\\\//g' -e 's/&/\\\&/g')//g")
    local PACKAGENAME=ndk-python-$(install_dir_from_host_tag $1).tar.bz2
    local PACKAGE="$PACKAGE_DIR/$PACKAGENAME"

    need_install_host_python $1 $2

    dump "$(bh_host_text) $PACKAGENAME: Packaging"
    run pack_archive "$PACKAGE" "$BLDDIR" "$SRCDIR"
}

need_package_host_python ()
{
    bh_stamps_do package-host-python-$1-$2 package_host_python $1 $2
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
        if [ $AUTO_BUILD != "yes" -o $SYSTEM != $BH_BUILD_TAG ]; then
            for VERSION in $PYTHON_VERSION; do
                need_package_host_python $SYSTEM $VERSION
            done
        fi
    done
fi
