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
#  This shell script is used to rebuild the gcc and toolchain binaries
#  for the Android NDK.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <ndk-dir> <toolchain>"

PROGRAM_DESCRIPTION=\
"Rebuild the gcc toolchain prebuilt binaries for the Android NDK.

Where <src-dir> is the location of toolchain sources, <ndk-dir> is
the top-level NDK installation path and <toolchain> is the name of
the toolchain to use (e.g. arm-linux-androideabi-4.6)."

RELEASE=`date +%Y%m%d`
BUILD_OUT=/tmp/ndk-$USER/build/toolchain
OPTION_BUILD_OUT=
register_var_option "--build-out=<path>" OPTION_BUILD_OUT "Set temporary build directory"

# Note: platform API level 9 or higher is needed for proper C++ support
PLATFORM=$DEFAULT_PLATFORM
register_var_option "--platform=<name>"  PLATFORM "Specify platform name"

OPTION_SYSROOT=
register_var_option "--sysroot=<path>"   OPTION_SYSROOT   "Specify sysroot directory directly"

GDB_VERSION=$DEFAULT_GDB_VERSION
EXPLICIT_GDB_VERSION=
register_option "--gdb-version=<version>"  do_gdb_version "Specify gdb version" "$GDB_VERSION"
do_gdb_version () {
    GDB_VERSION=$1
    EXPLICIT_GDB_VERSION=true
}

BINUTILS_VERSION=$DEFAULT_BINUTILS_VERSION
EXPLICIT_BINUTILS_VERSION=
register_option "--binutils-version=<version>" do_binutils_version "Specify binutils version" "$BINUTILS_VERSION"
do_binutils_version () {
    BINUTILS_VERSION=$1
    EXPLICIT_BINUTILS_VERSION=true
}

GMP_VERSION=$DEFAULT_GMP_VERSION
register_var_option "--gmp-version=<version>" GMP_VERSION "Specify gmp version"

MPFR_VERSION=$DEFAULT_MPFR_VERSION
register_var_option "--mpfr-version=<version>" MPFR_VERSION "Specify mpfr version"

MPC_VERSION=$DEFAULT_MPC_VERSION
register_var_option "--mpc-version=<version>" MPC_VERSION "Specify mpc version"

CLOOG_VERSION=$DEFAULT_CLOOG_VERSION
register_var_option "--cloog-version=<version>" CLOOG_VERSION "Specify cloog version"

ISL_VERSION=$DEFAULT_ISL_VERSION
register_var_option "--isl-version=<version>" ISL_VERSION "Specify ISL version"

PPL_VERSION=$DEFAULT_PPL_VERSION
register_var_option "--ppl-version=<version>" PPL_VERSION "Specify ppl version"

WITH_PYTHON=
register_var_option "--with-python=<path/to/python-config.sh>" WITH_PYTHON "Specify python config script, or prebuilt"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Create archive tarball in specific directory"

register_jobs_option
register_canadian_option
register_try64_option

extract_parameters "$@"

prepare_canadian_toolchain /tmp/ndk-$USER/build

fix_option BUILD_OUT "$OPTION_BUILD_OUT" "build directory"
setup_default_log_file $BUILD_OUT/config.log

set_parameters ()
{
    SRC_DIR="$1"
    NDK_DIR="$2"
    TOOLCHAIN="$3"

    # Check source directory
    #
    if [ -z "$SRC_DIR" ] ; then
        echo "ERROR: Missing source directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$SRC_DIR/gcc" ] ; then
        echo "ERROR: Source directory does not contain gcc sources: $SRC_DIR"
        exit 1
    fi
    SRC_DIR=`cd $SRC_DIR; pwd`
    log "Using source directory: $SRC_DIR"

    # Check NDK installation directory
    #
    if [ -z "$NDK_DIR" ] ; then
        echo "ERROR: Missing NDK directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$NDK_DIR" ] ; then
        mkdir -p $NDK_DIR
        if [ $? != 0 ] ; then
            echo "ERROR: Could not create target NDK installation path: $NDK_DIR"
            exit 1
        fi
    fi
    NDK_DIR=`cd $NDK_DIR; pwd`
    log "Using NDK directory: $NDK_DIR"

    # Check toolchain name
    #
    if [ -z "$TOOLCHAIN" ] ; then
        echo "ERROR: Missing toolchain name parameter. See --help for details."
        exit 1
    fi
}

set_parameters $PARAMETERS

# Disable x86_64 build for toolchains older than 4.7
case "$TOOLCHAIN" in
  x86_64-4.4.3|x86_64-4.6)
    echo "ERROR: x86_64 toolchain is enabled in 4.7+. Please try to build newer version."
    exit 1
    ;;
esac

prepare_target_build

parse_toolchain_name $TOOLCHAIN

fix_sysroot "$OPTION_SYSROOT"

check_toolchain_src_dir "$SRC_DIR"

if [ ! -d $SRC_DIR/gdb/gdb-$GDB_VERSION ] ; then
    echo "ERROR: Missing gdb sources: $SRC_DIR/gdb/gdb-$GDB_VERSION"
    echo "       Use --gdb-version=<version> to specify alternative."
    exit 1
fi

if [ -z "$EXPLICIT_BINUTILS_VERSION" ]; then
    BINUTILS_VERSION=$(get_default_binutils_version_for_gcc $TOOLCHAIN)
    dump "Auto-config: --binutils-version=$BINUTILS_VERSION"
fi

if [ ! -d $SRC_DIR/binutils/binutils-$BINUTILS_VERSION ] ; then
    echo "ERROR: Missing binutils sources: $SRC_DIR/binutils/binutils-$BINUTILS_VERSION"
    echo "       Use --binutils-version=<version> to specify alternative."
    exit 1
fi

if [ -z "$EXPLICIT_GDB_VERSION" ]; then
    GDB_VERSION=$(get_default_gdb_version_for_gcc $TOOLCHAIN)
    dump "Auto-config: --gdb-version=$GDB_VERSION"
fi

if [ ! -d $SRC_DIR/gdb/gdb-$GDB_VERSION ] ; then
    echo "ERROR: Missing gdb sources: $SRC_DIR/gdb/gdb-$GDB_VERSION"
    echo "       Use --gdb-version=<version> to specify alternative."
    exit 1
fi

if [ ! -z "$WITH_PYTHON" ] ; then
    if [ "$WITH_PYTHON" = "prebuilt" ] ; then
        WITH_PYTHON_SCRIPT="$ANDROID_NDK_ROOT/prebuilt/$HOST_TAG/bin/python-config.sh"
    fi
    if [ ! -f "$WITH_PYTHON_SCRIPT" ] ; then
        echo "ERROR: --with-python ($WITH_PYTHON_SCRIPT)"
        echo "       Does not exist!"
        exit 1
    else
        WITH_PYTHON="--with-python=$WITH_PYTHON_SCRIPT"
    fi
fi

fix_option MPFR_VERSION "$OPTION_MPFR_VERSION" "mpfr version"
if [ ! -f $SRC_DIR/mpfr/mpfr-$MPFR_VERSION.tar.bz2 ] ; then
    echo "ERROR: Missing mpfr sources: $SRC_DIR/mpfr/mpfr-$MPFR_VERSION.tar.bz2"
    echo "       Use --mpfr-version=<version> to specify alternative."
    exit 1
fi

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
fi

set_toolchain_ndk $NDK_DIR $TOOLCHAIN

if [ "$MINGW" != "yes" -a "$DARWIN" != "yes" ] ; then
    dump "Using C compiler: $CC"
    dump "Using C++ compiler: $CXX"
fi

rm -rf $BUILD_OUT
mkdir -p $BUILD_OUT

# Location where the toolchain license files are
TOOLCHAIN_LICENSES=$ANDROID_NDK_ROOT/build/tools/toolchain-licenses

# Without option "--sysroot" (and its variations), GCC will attempt to
# search path specified by "--with-sysroot" at build time for headers/libs.
# Path at --with-sysroot contains minimal headers and libs to boostrap
# toolchain build, and it's not needed afterward (NOTE: NDK provides
# sysroot at specified API level,and Android build explicit lists header/lib
# dependencies.
#
# It's better to point --with-sysroot to local directory otherwise the
# path may be found at compile-time and bad things can happen: eg.
#  1) The path exists and contain incorrect headers/libs
#  2) The path exists at remote server and blocks GCC for seconds
#  3) The path exists but not accessible, which crashes GCC!
#
# For canadian build --with-sysroot has to be sub-directory of --prefix.
# Put TOOLCHAIN_BUILD_PREFIX to BUILD_OUT which is in /tmp by default,
# and TOOLCHAIN_BUILD_SYSROOT underneath.

TOOLCHAIN_BUILD_PREFIX=$BUILD_OUT/prefix
TOOLCHAIN_BUILD_SYSROOT=$TOOLCHAIN_BUILD_PREFIX/sysroot
dump "Sysroot  : Copying: $SYSROOT --> $TOOLCHAIN_BUILD_SYSROOT"
mkdir -p $TOOLCHAIN_BUILD_SYSROOT && (cd $SYSROOT && tar ch *) | (cd $TOOLCHAIN_BUILD_SYSROOT && tar x)
if [ $? != 0 ] ; then
    echo "Error while copying sysroot files. See $TMPLOG"
    exit 1
fi

# configure the toolchain
#
dump "Configure: $TOOLCHAIN toolchain build"
# Old versions of the toolchain source packages placed the
# configure script at the top-level. Newer ones place it under
# the build directory though. Probe the file system to check
# this.
BUILD_SRCDIR=$SRC_DIR/build
if [ ! -d $BUILD_SRCDIR ] ; then
    BUILD_SRCDIR=$SRC_DIR
fi
OLD_ABI="${ABI}"
export CC CXX
export CFLAGS_FOR_TARGET="$ABI_CFLAGS_FOR_TARGET"
export CXXFLAGS_FOR_TARGET="$ABI_CXXFLAGS_FOR_TARGET"
# Needed to build a 32-bit gmp on 64-bit systems
export ABI=$HOST_GMP_ABI

# Note that the following flags only apply for "build" in canadian
# -Wno-error is needed because our gdb-6.6 sources use -Werror by default
# and fail to build with recent GCC versions.
CFLAGS_FOR_BUILD="-O2 -s -Wno-error"
LDFLAGS_FOR_BUILD=

if [ "$MINGW" = "yes" ] ; then
    CFLAGS_FOR_BUILD=$CFLAGS_FOR_BUILD" -D__USE_MINGW_ANSI_STDIO=1"
fi

CFLAGS="$CFLAGS_FOR_BUILD $HOST_CFLAGS"
LDFLAGS="$LDFLAGS_FOR_BUILD $HOST_LDFLAGS"

export CFLAGS LDFLAGS CFLAGS_FOR_BUILD LDFLAGS_FOR_BUILD

# This extra flag is used to slightly speed up the build
EXTRA_CONFIG_FLAGS="--disable-bootstrap"

# This is to disable GCC 4.6 specific features that don't compile well
# the flags are ignored for older GCC versions.
EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --disable-libquadmath"
if [ "$DARWIN" = "yes" ]; then
    # Disable plugin because in canadian cross build, plugin gengtype
    # will be incorrectly linked with build's library and fails.
    # ToDo
    EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --disable-plugin"
else
    # Plugins are not supported well before 4.7. On 4.7 it's required to have
    # -flto working. Flag --enable-plugins (note 's') is actually for binutils,
    # this is compiler requirement to have binutils configured this way. Flag
    # --disable-plugin is for gcc.
    case "$GCC_VERSION" in
        4.4.3)
            EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --disable-plugin"
            ;;
        *)
            EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --enable-plugins"
            ;;
    esac
fi

# Enable OpenMP
case "$TOOLCHAIN" in
    *) EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --enable-libgomp" ;;
esac

# Disable libsanitizer (which depends on libstdc++ built separately) for now
EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --disable-libsanitizer"

# Enable Gold as default
case "$TOOLCHAIN" in
    # Note that only ARM and X86 >= GCC 4.6 are supported
    mips*)
    ;;
    *-4.4.3)
    ;;
    *)
        EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --enable-gold=default"
    ;;
esac

# Enable Graphite
case "$TOOLCHAIN" in
    *-4.4.3) ;;
    *-4.6|*-4.7)
        EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --enable-graphite=yes --with-cloog-version=$CLOOG_VERSION --with-ppl-version=$PPL_VERSION"
    ;;
    *)
        EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --enable-graphite=yes --with-cloog-version=$CLOOG_VERSION --with-isl-version=$ISL_VERSION"
    ;;
esac

# Enable linker option -eh-frame-hdr also for static executable
EXTRA_CONFIG_FLAGS=$EXTRA_CONFIG_FLAGS" --enable-eh-frame-hdr-for-static"

cd $BUILD_OUT && run \
$BUILD_SRCDIR/configure --target=$ABI_CONFIGURE_TARGET \
                        --enable-initfini-array \
                        --host=$ABI_CONFIGURE_HOST \
                        --build=$ABI_CONFIGURE_BUILD \
                        --disable-nls \
                        --prefix=$TOOLCHAIN_BUILD_PREFIX \
                        --with-sysroot=$TOOLCHAIN_BUILD_SYSROOT \
                        --with-binutils-version=$BINUTILS_VERSION \
                        --with-mpfr-version=$MPFR_VERSION \
                        --with-mpc-version=$MPC_VERSION \
                        --with-gmp-version=$GMP_VERSION \
                        --with-gcc-version=$GCC_VERSION \
                        --with-gdb-version=$GDB_VERSION \
                        $WITH_PYTHON \
                        --with-gxx-include-dir=$TOOLCHAIN_BUILD_PREFIX/include/c++/$GCC_VERSION \
                        --with-bugurl=$DEFAULT_ISSUE_TRACKER_URL \
                        $EXTRA_CONFIG_FLAGS \
                        $ABI_CONFIGURE_EXTRA_FLAGS
if [ $? != 0 ] ; then
    dump "Error while trying to configure toolchain build. See $TMPLOG"
    exit 1
fi

ABI="$OLD_ABI"
# build the toolchain
dump "Building : $TOOLCHAIN toolchain [this can take a long time]."
cd $BUILD_OUT
export CC CXX
export ABI=$HOST_GMP_ABI
JOBS=$NUM_JOBS

while [ -n "1" ]; do
    run make -j$JOBS
    if [ $? = 0 ] ; then
        break
    else
        if [ "$MINGW" = "yes" -o "$DARWIN" = "yes" ] ; then
            # Unfortunately, there is a bug in the GCC build scripts that prevent
            # parallel mingw/darwin canadian cross builds to work properly on some
            # multi-core machines (but not all, sounds like a race condition). Detect
            # this and restart in less parallelism, until -j1 also fail
            JOBS=$((JOBS/2))
            if [ $JOBS -lt 1 ] ; then
                echo "Error while building mingw/darwin toolchain. See $TMPLOG"
                exit 1
            fi
            dump "Parallel canadian build failed - continuing in less parallelism -j$JOBS"
        else
            echo "Error while building toolchain. See $TMPLOG"
            exit 1
        fi
    fi
done

ABI="$OLD_ABI"

# install the toolchain to its final location
dump "Install  : $TOOLCHAIN toolchain binaries."
cd $BUILD_OUT && run make install
if [ $? != 0 ] ; then
    echo "Error while installing toolchain. See $TMPLOG"
    exit 1
fi

# copy to toolchain path
run copy_directory "$TOOLCHAIN_BUILD_PREFIX" "$TOOLCHAIN_PATH"

if [ "$MINGW" = "yes" -o "$DARWIN" = "yes" ] ; then
    # For some reasons, libraries in $ABI_CONFIGURE_TARGET (*) are not installed.
    # Hack here to copy them over.
    # (*) FYI: libgcc.a and libgcov.a not installed there in the first place
    INSTALL_TARGET_LIB_PATH="$BUILD_OUT/host-$ABI_CONFIGURE_BUILD/install/$ABI_CONFIGURE_TARGET/lib"
    TOOLCHAIN_TARGET_LIB_PATH="$TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib"
    (cd "$INSTALL_TARGET_LIB_PATH" &&
        find . \( -name "*.a" -o -name "*.la" -o -name "*.spec" \) -exec install -D "{}" "$TOOLCHAIN_TARGET_LIB_PATH/{}" \;)
fi

# build the gdb stub and replace gdb with it. This is done post-install
# so files are in the correct place when determining the relative path.
if [ "$MINGW" = "yes" ] ; then
    WITH_PYTHON_PREFIX=$(dirname $(dirname "$WITH_PYTHON_SCRIPT"))
    dump "Building : $TOOLCHAIN GDB stub. "$TOOLCHAIN_PATH/bin/${ABI_CONFIGURE_TARGET}-gdb.exe", "$WITH_PYTHON_PREFIX", $ABI_CONFIGURE_HOST-gcc"
    GCC_FOR_STUB=$ABI_CONFIGURE_HOST-gcc
    if [ "$TRY64" != "yes" ]; then
        # The i586-mingw32msvc-gcc is missing CreateJobObject, SetInformationJobObject, and
        # AssignProcessToJobObject needed for gdb-stub.c.  Hack to use i686-w64-mingw32-gcc.  ToDo:
        GCC_FOR_STUB_TARGET=`$GCC_FOR_STUB -dumpmachine`
        if [ "$GCC_FOR_STUB_TARGET" = "i586-mingw32msvc" ]; then
            GCC_FOR_STUB=i686-w64-mingw32-gcc
            dump "Override compiler for gdb-stub: $GCC_FOR_STUB"
	fi
    fi
    run $NDK_DIR/build/tools/build-gdb-stub.sh --gdb-executable-path="$TOOLCHAIN_PATH/bin/${ABI_CONFIGURE_TARGET}-gdb.exe" \
                                               --python-prefix-dir=${WITH_PYTHON_PREFIX} \
                                               --mingw-w64-gcc=$GCC_FOR_STUB
fi

# don't forget to copy the GPL and LGPL license files
run cp -f $TOOLCHAIN_LICENSES/COPYING $TOOLCHAIN_LICENSES/COPYING.LIB $TOOLCHAIN_PATH

# remove some unneeded files
run rm -f $TOOLCHAIN_PATH/bin/*-gccbug
run rm -f $TOOLCHAIN_PATH/bin/*gdbtui$HOST_EXE
run rm -f $TOOLCHAIN_PATH/bin/*-run$HOST_EXE
run rm -rf $TOOLCHAIN_PATH/info
run rm -rf $TOOLCHAIN_PATH/man
run rm -rf $TOOLCHAIN_PATH/share/info
run rm -rf $TOOLCHAIN_PATH/share/man
run rm -rf $TOOLCHAIN_PATH/lib/gcc/$ABI_CONFIGURE_TARGET/*/install-tools
run rm -rf $TOOLCHAIN_PATH/lib/gcc/$ABI_CONFIGURE_TARGET/*/plugin
run rm -rf $TOOLCHAIN_PATH/libexec/gcc/$ABI_CONFIGURE_TARGET/*/install-tools
run rm -rf $TOOLCHAIN_PATH/lib/libiberty.a
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/libiberty.a
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/*/libiberty.a
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/*/*/libiberty.a
find $TOOLCHAIN_PATH -name "*.la" -exec rm -f {} \;

# Remove libstdc++ for now (will add it differently later)
# We had to build it to get libsupc++ which we keep.
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/libstdc++.*
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/lib/*/libstdc++.*
run rm -rf $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/include/c++

# strip binaries to reduce final package size
test -z "$STRIP" && STRIP=strip
run $STRIP $TOOLCHAIN_PATH/bin/*
run $STRIP $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/bin/*
run $STRIP $TOOLCHAIN_PATH/libexec/gcc/*/*/cc1$HOST_EXE
run $STRIP $TOOLCHAIN_PATH/libexec/gcc/*/*/cc1plus$HOST_EXE
run $STRIP $TOOLCHAIN_PATH/libexec/gcc/*/*/collect2$HOST_EXE
run $STRIP $TOOLCHAIN_PATH/libexec/gcc/*/*/lto*$HOST_EXE

# Some of the files should really be links to save space.
# This is mostly to reduce the size of the Windows zip archives,
# since:
#  - The toolchain install script actually use hard-links
#  - Tar automatically detects hard links and will only store a
#    single copy of each file anyway.

# $1: Source file (will be changed to a link)
# $2: Destination (relative to source).
do_relink () {
    log "Relink: $1 --> $2"
    local BASENAME DIRNAME
    DIRNAME=$(dirname "$1")
    BASENAME=$(basename "$1")
    ( cd "$DIRNAME" && rm -f "$BASENAME" && ln -s "$2" "$BASENAME" )
    fail_panic "Can't relink $1 to $2"
}

# <config>/bin/<name> should point to ../../<config>-<name>
LINK_FILES=$(cd $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/bin && ls * 2>/dev/null)
for LINK_FILE in $LINK_FILES; do
  do_relink $TOOLCHAIN_PATH/$ABI_CONFIGURE_TARGET/bin/$LINK_FILE ../../bin/$ABI_CONFIGURE_TARGET-$LINK_FILE
done

# $1: Source file prefix (e.g. 'c++')
# $2: Destination file prefix (e.g. 'g++')
# $3: Alternative file prefix if $2 doesn't exist (eg. ld.bfd)
do_relink_bin () {
    local DST_FILE=$2
    if [ ! -f "$TOOLCHAIN_PATH/bin/$ABI_CONFIGURE_TARGET-$DST_FILE$HOST_EXE" ]; then
        DST_FILE=$3
    fi
    if [ ! -f "$TOOLCHAIN_PATH/bin/$ABI_CONFIGURE_TARGET-$DST_FILE$HOST_EXE" ]; then
        echo "ERROR: Can't relink $1 to $DST_FILE because $DST_FILE doesn't exist"
        exit 1
    fi
    do_relink \
        $TOOLCHAIN_PATH/bin/$ABI_CONFIGURE_TARGET-$1$HOST_EXE \
        $ABI_CONFIGURE_TARGET-$DST_FILE$HOST_EXE
}

do_relink_bin c++ g++
do_relink_bin gcc-$GCC_VERSION gcc
# symlink ld to either ld.gold or ld.bfd
do_relink_bin ld ld.gold ld.bfd

# copy SOURCES file if present
if [ -f "$SRC_DIR/SOURCES" ]; then
    cp "$SRC_DIR/SOURCES" "$TOOLCHAIN_PATH/SOURCES"
fi

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE="$TOOLCHAIN-$HOST_TAG.tar.bz2"
    SUBDIR=$(get_toolchain_install_subdir $TOOLCHAIN $HOST_TAG)
    dump "Packaging $ARCHIVE"
  # exlude ld.mcld
    EXCLUSIONS=
    if [ -f $SUBDIR/bin/$ABI_CONFIGURE_TARGET-ld.mcld${HOST_EXE} ] ; then
        EXCLUSIONS=$EXCLUSIONS" --exclude=$SUBDIR/bin/$ABI_CONFIGURE_TARGET-ld.mcld${HOST_EXE}"
    fi
    if [ -f $SUBDIR/$ABI_CONFIGURE_TARGET/bin/ld.mcld${HOST_EXE} ] ; then
        EXCLUSIONS=$EXCLUSIONS" --exclude=$SUBDIR/$ABI_CONFIGURE_TARGET/bin/ld.mcld${HOST_EXE}"
    fi
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SUBDIR" $EXCLUSIONS
fi

dump "Done."
if [ -z "$OPTION_BUILD_OUT" ] ; then
    rm -rf $BUILD_OUT
fi
