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
# Rebuild the host GCC toolchain binaries from sources.
#
# NOTE: this script does not rebuild gdb, see build-host-gdb.sh for this.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

BINUTILS_VERSION=$DEFAULT_BINUTILS_VERSION
register_var_option "--binutils-version=<version>" BINUTILS_VERSION "Select binutils version"

GMP_VERSION=$DEFAULT_GMP_VERSION
register_var_option "--gmp-version=<version>" GMP_VERSION "Select gmp version"

MPFR_VERSION=$DEFAULT_MPFR_VERSION
register_var_option "--mpfr-version=<version>" MPFR_VERSION "Select mpfr version"

MPC_VERSION=$DEFAULT_MPC_VERSION
register_var_option "--mpc-version=<version>" MPC_VERSION "Select mpc version"

TOOLCHAIN_SRC_DIR=
register_var_option "--toolchain-src-dir=<path>" TOOLCHAIN_SRC_DIR "Select toolchain source directory"

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Select NDK install directory"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package prebuilt tarballs into directory"

TARGET_FILES=
register_var_option "--target-files" ENABLE_TARGET "Build target files (libgcc et al) too."

HOST_SYSTEMS="$HOST_TAG"
register_var_option "--systems=<list>" HOST_SYSTEMS "Build binaries for these host systems"

FORCE=
register_var_option "--force" FORCE "Force full rebuild"

NO_TARGET_LIBS=
register_var_option "--no-target-libs" NO_TARGET_LIBS "Don't build gcc target libs."

NO_STRIP=
register_var_option "--no-strip" NO_STRIP "Don't strip generated binaries."

register_jobs_option


extract_parameters "$@"

TOOLCHAINS=$PARAMETERS
if [ -z "$TOOLCHAINS" ]; then
    TOOLCHAINS="arm-linux-androideabi-4.4.3,arm-linux-androideabi-4.6,x86-4.4.3,x86-4.6,mipsel-linux-android-4.4.3,mipsel-linux-android-4.6"
    dump "Auto-config: $TOOLCHAINS"
fi

if [ -z "$TOOLCHAIN_SRC_DIR" ]; then
    panic "Please use --toolchain-src-dir=<path> to select toolchain source directory."
fi

HOST_SYSTEMS=$(commas_to_spaces $HOST_SYSTEMS)
TOOLCHAINS=$(commas_to_spaces $TOOLCHAINS)

# The values of HOST_OS/ARCH/TAG will be redefined during the build to
# match those of the system the generated compiler binaries will run on.
#
# Save the original ones into BUILD_XXX variants, corresponding to the
# machine where the build happens.
#
BUILD_OS=$HOST_OS
BUILD_ARCH=$HOST_ARCH
BUILD_TAG=$HOST_TAG

# $1: system tag (e.g. linux-x86)
tag_to_os ()
{
    local RET
    case $1 in
        linux-*) RET="linux";;
        darwin-*) RET="darwin";;
        windows|windows-*) RET="windows";;
    esac
    echo $RET
}

# $1: system tag (e.g. linux-x86)
tag_to_arch ()
{
    local RET
    case $1 in
        windows|*-x86) RET=x86;;
        *-x86_64) RET=x86_64;;
    esac
    echo $RET
}

# $1: system tag (e.g. linux-x86)
tag_to_bits ()
{
    local RET
    case $1 in
        windows|*-x86) RET=32;;
        *-x86_64) RET=64;;
    esac
    echo $RET
}

# We're going to cheat a little here. If we're only building a linux-x86
# on a linux-x86_64 machine, we want to change the value of BUILD_TAG
# to linux-x86 instead to speed-up the build.
#
# More generally speaking, we need to verify that if:
#   - we build a $BUILD_OS-x86 toolchain on a $BUILD_OS-x86_64 machine
#   - we don't want to build $BUILD_OS-x86_64 either.
#
# Then we can change our BUILD values to $BUILD_OS-x86
# This assumes that the build machine's toolchain can generate both
# 32-bit and 64-bit binaries with either -m32 or -m64
#
BUILD_BUILD_32=
BUILD_BUILD_64=
for SYSTEM in $HOST_SYSTEMS; do
    if [ "$(tag_to_os $SYSTEM)" = "$BUILD_OS" ]; then
        BUILD_BUILD_OS=true
        case $(tag_to_bits $SYSTEM) in
            32) BUILD_BUILD_32=true;;
            64) BUILD_BUILD_64=true;;
        esac
    fi
done

case $(tag_to_bits $BUILD_TAG) in
    64)
        # Building on a 64-bit machine
        if [ "$BUILD_BUILD_32" -a -z "$BUILD_BUILD_64" ]; then
            # Ok, we want to build a 32-bit toolchain on a 64-bit machine
            # So cheat a little now :-)
            BUILD_ARCH=x86
            BUILD_TAG=$BUILD_OS-$BUILD_ARCH
            dump "Forcing build config: $BUILD_TAG"
        fi
        ;;
esac

BUILD_BITS=$(tag_to_bits $BUILD_TAG)

# On Darwin, parallel installs of certain libraries do not work on
# some multi-core machines. So define NUM_BUILD_JOBS as 1 on this
# platform.
case $BUILD_OS in
    darwin) NUM_INSTALL_JOBS=1;;
    *) NUM_INSTALL_JOBS=$NUM_JOBS;;
esac

extract_version ()
{
    echo $1 | tr '-' '\n' | tail -1
}

# Given an input string of the form <foo>-<bar>-<version>, where
# <version> can be <major>.<minor>, extract <major>
#
# $1: versioned name (e.g. arm-linux-androideabi-4.4.3)
# Out: major version (e.g. 4)
#
# Examples:  arm-linux-androideabi-4.4.3 -> 4
#            gmp-0.81 -> 0
#
extract_major_version ()
{
    local RET=$(extract_version $1 | cut -d . -f 1)
    RET=${RET:-0}
    echo $RET
}

# Same as extract_major_version, but for the minor version number
# $1: versioned named
# Out: minor version
#
extract_minor_version ()
{
    local RET=$(extract_version $1 | cut -d . -f 2)
    RET=${RET:-0}
    echo $RET
}

# Compare two version numbers and only succeeds if the first one is
# greather or equal than the second one.
#
# $1: first version (e.g. 4.4.3)
# $2: second version (e.g. 4.6)
#
# Example: version_is_greater_than 4.6 4.4.3 --> success
#
version_is_greater_than ()
{
    local A_MAJOR A_MINOR B_MAJOR B_MINOR
    A_MAJOR=$(extract_major_version $1)
    B_MAJOR=$(extract_major_version $2)

    if [ $A_MAJOR -lt $B_MAJOR ]; then
        return 1
    elif [ $A_MAJOR -gt $B_MAJOR ]; then
        return 0
    fi

    # We have A_MAJOR == B_MAJOR here

    A_MINOR=$(extract_minor_version $1)
    B_MINOR=$(extract_minor_version $2)

    if [ $A_MINOR -lt $B_MINOR ]; then
        return 1
    else
        return 0
    fi
}

tag_to_config_triplet ()
{
    local RET
    case $1 in
        linux-x86) RET=i686-linux-gnu;;
        linux-x86_64) RET=x86_64-linux-gnu;;
        darwin-x86) RET=i686-apple-darwin;;
        darwin-x86_64) RET=x86_64-apple-darwin;;
        windows|windows-x86) RET=i586-pc-mingw32msvc;;
        windows-x86_64) RET=x86_64-w64-mingw32;;
    esac
    echo "$RET"
}

run_on_setup ()
{
    if [ "$PHASE" = setup ]; then
        run "$@"
    fi
}

setup_build ()
{
    BUILD_DIR=/tmp/ndk-$USER/build/host-gcc
    run_on_setup mkdir -p "$BUILD_DIR"
    if [ -n "$FORCE" ]; then
        rm -rf "$BUILD_DIR"/*
    fi

    TOP_BUILD_DIR=$BUILD_DIR

    setup_default_log_file $BUILD_DIR/build.log

    WRAPPERS_DIR="$BUILD_DIR/toolchain-wrappers"
    run_on_setup mkdir -p "$WRAPPERS_DIR" && run_on_setup rm -rf "$WRAPPERS_DIR/*"

    STAMPS_DIR="$BUILD_DIR/timestamps"
    run_on_setup mkdir -p "$STAMPS_DIR"
    if [ -n "$FORCE" ]; then
        run_on_setup rm -f "$STAMPS_DIR"/*
    fi

    BUILD=$(tag_to_config_triplet $BUILD_TAG)
}

stamps_do ()
{
    local NAME=$1
    shift
    if [ ! -f "$STAMPS_DIR/$NAME" ]; then
        "$@"
        fail_panic
        mkdir -p "$STAMPS_DIR" && touch "$STAMPS_DIR/$NAME"
    fi
}

get_default_binutils_version_for_gcc ()
{
    local RET
    case $1 in
        arm-*-4.4.3|x86-*-4.4.3) RET=2.19;;
        *) RET=2.21;;
    esac
    echo "$RET"
}

try_host_fullprefix ()
{
    if [ -z "$HOST_FULLPREFIX" ]; then
        if [ -f "$1/bin/$2-gcc" ]; then
            HOST_FULLPREFIX="$1/bin/$2-"
            log "Using host gcc: ${HOST_FULLPREFIX}gcc"
        fi
    fi
}

try_host_prefix ()
{
    if [ -z "$HOST_FULLPREFIX" ]; then
        local GCC="$(which $1-gcc 2>/dev/null)"
        if [ "$GCC" -a -e "$GCC" ]; then
            GCC=$(which $1-gcc)
            HOST_FULLPREFIX=${GCC%%gcc}
            log "Using host gcc: ${HOST_FULLPREFIX}gcc"
        fi
    fi
}

# Use the check for the availability of a compatibility SDK in Darwin
# this can be used to generate binaries compatible with either Tiger or
# Leopard.
#
# $1: SDK root path
# $2: MacOS X minimum version (e.g. 10.4)
check_darwin_sdk ()
{
    if [ -d "$1" ] ; then
        HOST_CFLAGS="-isysroot $1 -mmacosx-version-min=$2 -DMAXOSX_DEPLOYEMENT_TARGET=$2"
        HOST_CXXFLAGS=$HOST_CFLAGS
        HOST_LDFLAGS="-Wl,-syslibroot,$sdk -mmacosx-version-min=$2"
        if [ "$PHASE" = setup ]; then
            dump "Generating $2-compatible binaries."
        fi
        return 0  # success
    fi
    return 1
}

# Check that a given compiler generates 32 or 64 bit code.
# $1: compiler full path (.e.g  /path/to/fullprefix-gcc)
# $2: 32 or 64
# $3: extract compiler flags
# Return: success iff the compiler generates $2-bits code
check_compiler_bitness ()
{
    local CC="$1"
    local BITS="$2"
    local TMPC=/tmp/build-host-gcc-bits-$USER-$$.c
    local TMPL=$TMPC.log
    local RET
    shift; shift;
    cat > $TMPC <<EOF
/* this program will fail to compile if the compiler doesn't generate BITS-bits code */
int tab[1-2*(sizeof(void*)*8 != BITS)];
EOF
    log -n "Checking that the compiler generates $BITS-bits code ($@)... "
    $CC -c -DBITS=$BITS -o /dev/null $TMPC $@ > $TMPL 2>&1
    RET=$?
    rm -f $TMPC $TMPL
    if [ "$RET" = 0 ]; then
        log "yes"
    else
        log "no"
    fi
    return $RET
}

# Call this before anything else to setup a few important variables that are
# used consistently to build any host-specific binaries.
#
# $1: Host system name (e.g. linux-x86), this is the name of the host system
#     where the generated GCC binaries will run, not the current machine's
#     type (this one is in $ORIGINAL_HOST_TAG instead).
#
setup_build_for_host ()
{
    local HOST_CFLAGS HOST_CXXFLAGS HOST_LDFLAGS HOST_FULLPREFIX

    # Determine the host configuration triplet in $HOST
    HOST=$(tag_to_config_triplet $1)
    HOST_OS=$(tag_to_os $1)
    HOST_ARCH=$(tag_to_arch $1)
    HOST_BITS=$(tag_to_bits $1)
    HOST_TAG=$1

    # We do all the complex auto-detection magic in the setup phase,
    # then save the result in host-specific global variables.
    #
    # In the build phase, we will simply restore the values into the
    # global HOST_FULLPREFIX / HOST_BUILD_DIR
    # variables.
    #

    # Try to find the best toolchain to do that job, assuming we are in
    # a full Android platform source checkout, we can look at 
    case $1 in
        linux-x86)
            # If possible, automatically use our custom toolchain to generate
            # 32-bit executables that work on Ubuntu 8.04 and higher.
            try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/i686-linux-glibc2.7-4.6" i686-linux
            try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/i686-linux-glibc2.7-4.4.3" i686-linux
            try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilt/linux-x86/toolchain/i686-linux-glibc2.7-4.4.3" i686-linux
            try_host_prefix i686-linux-gnu
            try_host_prefix i686-linux
            ;;

        linux-x86_64)
            # If possible, automaticaly use our custom toolchain to generate
            # 64-bit executables that work on Ubuntu 8.04 and higher.
            try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.7-4.6" x86_64-linux
            try_host_prefix x86_64-linux-gnu
            try_host_prefix x84_64-linux
            ;;

        darwin-*)
            # For now, we can only build Darwin binaries on build machines :-(
            if [ "$BUILD_OS" != "darwin" ]; then
                panic "Sorry, generating Darwin binaries requires a Darwin build machine!"
            fi
            # Try to build with Tiger SDK if available
            check_darwin_sdk /Developer/SDKs/MacOSX10.4.sdku 10.4
            check_darwin_sdk /Developer/SDKs/MacOSX10.5.sdk  10.5
            check_darwin_sdk /Developer/SDKs/MacOSX10.6.sdk  10.6
            if [ -z "$HOST_CFLAGS" -a "$PHASE" = setup ]; then
                local version="$(sw_vers -productVersion)"
                log "Generating $version-compatible binaries!"
            fi
            ;;

        windows|windows-*)
            case $BUILD_OS in
                linux)
                    # We need either i586-mingw32msvc-gcc,
                    # i686-w64-mingw32-gcc or x86_64-w64-mingw32-gcc
                    # to build Windows binaries on Linux
                    try_host_prefix x86_64-w64-mingw32
                    try_host_prefix i686-w64-mingw32
                    try_host_prefix i586-mingw32msvc
                    if [ -z "$HOST_FULLPREFIX" ]; then
                        dump "There is no Windows cross-compiler in your path. Ensure you"
                        dump "have one of these installed and in your path:"
                        dump "   x86_64-w64-mingw32-gcc"
                        dump "   i686-w64-mingw32-gcc"
                        dump "   i586-mingw32msvc-gcc"
                        dump ""
                        exit 1
                    fi
                    case $HOST_FULLPREFIX in
                        *-mingw32msvc-*)
                            HOST=i586-pc-mingw32msvc
                            ;;
                        *) case $1 in
                                windows-x86_64)
                                    HOST=x86_64-w64-mingw32
                                    ;;
                                *)
                                    HOST=i686-w64-mingw32
                                    ;;
                            esac
                            ;;
                    esac
                    ;;

                *) panic "Sorry, this script only supports building windows binaries on Linux."
                ;;
            esac
            ;;
    esac

    if [ "$PHASE" = setup ]; then
        mkdir -p "$(host_build_dir)"
        if [ "$FORCE" ]; then
            rm -rf "$(host_build_dir)"/*
        fi

        # Determine the default bitness of our compiler. It it doesn't match
        # HOST_BITS, tries to see if it supports -m32 or -m64 to change it.
        if ! check_compiler_bitness ${HOST_FULLPREFIX}gcc $HOST_BITS; then
            TRY_CFLAGS=
            case $HOST_BITS in
                32) TRY_CFLAGS=-m32;;
                64) TRY_CFLAGS=-m64;;
            esac
            if ! check_compiler_bitness ${HOST_FULLPREFIX}gcc $HOST_BITS $TRY_CFLAGS; then
                panic "Can't find a way to generate $HOST_BITS binaries with this compiler: ${HOST_FULLPREFIX}gcc"
            else
                log "Using flag to force $HOST_BITS-bit code generation: $TRY_CFLAGS"
            fi
            HOST_CFLAGS=$TRY_CFLAGS
            HOST_CXXFLAGS=$TRY_CFLAGS
        fi

        # Support for ccache, to speed up rebuilds.
        DST_PREFIX=$HOST_FULLPREFIX
        if [ "$NDK_CCACHE" ]; then
            DST_PREFIX="$NDK_CCACHE $HOST_FULLPREFIX"
        fi

        # We're going to generate a wrapper toolchain with the $HOST prefix
        # i.e. if $HOST is 'i686-linux-gnu', then we're going to generate a
        # wrapper toolchain named 'i686-linux-gnu-gcc' that will redirect
        # to whatever HOST_FULLPREFIX points to, with appropriate modifier
        # compiler/linker flags.
        #
        # This helps tremendously getting stuff to compile with the GCC
        # configure scripts.
        #
        run $NDK_BUILDTOOLS_PATH/gen-toolchain-wrapper.sh "$WRAPPERS_DIR" \
            --src-prefix="$HOST-" \
            --dst-prefix="$DST_PREFIX" \
            --cflags="$HOST_CFLAGS" \
            --cxxflags="$HOST_CXXFLAGS" \
            --ldflags="$HOST_LDFLAGS"
    fi
}

# Returns the location of all $HOST specific files (build and install)
host_build_dir ()
{
    echo "$TOP_BUILD_DIR/$HOST"
}

# Return the location of the build directory for a specific component
# $1: component name (e.g. gmp-4.2.4)
host_build_dir_for ()
{
    echo "$(host_build_dir)/build-$1"
}

# Returns the install location of the $HOST pre-reqs libraries
host_prereqs_install_dir ()
{
    echo "$(host_build_dir)/temp-prereqs"
}

# Returns the install location of the $HOST binutils cross-toolchain
host_binutils_install_dir ()
{
    echo "$(host_build_dir)/temp-binutils-$BINUTILS_VERSION-$TARGET"
}

# Returns the install location of the $HOST binutils cross-toolchain
build_binutils_install_dir ()
{
    echo "$TOP_BUILD_DIR/$BUILD/temp-binutils-$BINUTILS_VERSION-$TARGET"
}

# Returns the install location of the $HOST gcc cross-toolchain
host_gcc_install_dir ()
{
    echo "$(host_build_dir)/temp-$TOOLCHAIN"
}

# Returns the install location of the $BUILD gcc cross-toolchain
build_gcc_install_dir ()
{
    echo "$TOP_BUILD_DIR/$BUILD/temp-$TOOLCHAIN"
}


# Location of the host sysroot used during the build
host_sysroot ()
{
    # This must be a sub-directory of $(host_gcc_install_dir)
    # to generate relocatable binaries that are used by
    # standalone versions of the toolchain.
    #
    # If you change this, you will need to modify make-standalone-toolchain.sh
    # as well.
    #
    echo "$(host_gcc_install_dir)/sysroot"
}

# Returns the final install location of the $HOST toolchain
# This ones contains the binutils binaries, the gcc ones,
# the target libraries, but does *not* include the sysroot
# and other stuff (e.g. documentation like info or man files).
#
host_gcc_final_dir ()
{
    echo "$(host_build_dir)/final-$TOOLCHAIN"
}

setup_build_for_toolchain ()
{
    GCC_VERSION=$(extract_version $1)
    BINUTILS_VERSION=$(get_default_binutils_version_for_gcc $1)

    TARGET_ARCH=$(echo $1 | cut -d - -f 1)

    # NOTE: The 'mipsel' toolchain architecture name maps to the 'mips'
    # NDK architecture name.
    case $TARGET_ARCH in
        arm) TARGET=arm-linux-androideabi;;
        x86) TARGET=i686-linux-android;;
        mips|mipsel) TARGET=mipsel-linux-android; TARGET_ARCH=mips;;
        *) panic "Unknown target toolchain architecture: $TARGET_ARCH"
    esac

    TARGET_SYSROOT=$ANDROID_NDK_ROOT/$(get_default_platform_sysroot_for_arch $TARGET_ARCH)
    if [ ! -d "$TARGET_SYSROOT" ]; then
        panic "Missing directory: $TARGET_SYSROOT"
    fi

    # MPC is only needed starting with GCC 4.5
    HOST_NEED_MPC=
    if version_is_greater_than $GCC_VERSION 4.5; then
        HOST_NEED_MPC=true
    fi

    # TODO: We will need to place these under
    #      $NDK_DIR/prebuilts/$HOST/android-$TARGET_ARCH-gcc-$GCC_VERSION/
    #      in a future patch.
    TOOLCHAIN_INSTALL_DIR=$NDK_DIR/toolchains/$TOOLCHAIN/prebuilt/$HOST_TAG

    # These will go into CFLAGS_FOR_TARGET and others during the build
    # of GCC target libraries.
    if [ -z "$NO_STRIP" ]; then
        TARGET_CFLAGS="-O2 -fomit-frame-pointer -s"
    else
        TARGET_CFLAGS="-O2 -Os -g"
    fi

    TARGET_CXXFLAGS=$TARGET_CFLAGS
    TARGET_LDFLAGS=""

    if [ "$TARGET_ARCH" = x86 ]; then
        TARGET_CFLAGS=$TARGET_CFLAGS" \
        -DANDROID -D__ANDROID__ -Ulinux \
        -fPIC -Wa,--noexecstack -m32 -fstack-protector \
        -W -Wall -Werror=address -Werror=format-security -Werror=non-virtual-dtor -Werror=return-type \
        -Werror=sequence-point -Winit-self -Wno-multichar -Wno-unused -Wpointer-arith -Wstrict-aliasing=2 \
        -fexceptions -ffunction-sections -finline-functions \
        -finline-limit=300 -fmessage-length=0 -fno-inline-functions-called-once \
        -fno-strict-aliasing -frtti \
        -fstrict-aliasing -funswitch-loops -funwind-tables \
        -march=i686 -mtune=atom -mbionic -mfpmath=sse -mstackrealign -DUSE_SSE2"

        TARGET_LDFLAGS=$TARGET_LDFLAGS" \
        -m32 -O2 -g -fPIC \
        -nostartfiles \
        -Wl,-z,noexecstack -Wl,--gc-sections -nostdlib \
        -fexceptions -frtti -fstrict-aliasing -ffunction-sections -finline-functions  \
        -finline-limit=300 -fno-inline-functions-called-once \
        -funswitch-loops -funwind-tables -mstackrealign \
        -ffunction-sections -funwind-tables -fmessage-length=0 \
        -march=atom -mstackrealign -mfpmath=sse -mbionic \
        -Wno-multichar -Wl,-z,noexecstack -Werror=format-security -Wstrict-aliasing=2 \
        -W -Wall -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type -Werror=non-virtual-dtor \
        -Werror=address -Werror=sequence-point \
        -Werror=format-security -Wl,--no-undefined \
        -nostartfiles $TARGET_SYSROOT/usr/lib/crtbegin_dynamic.o $TARGET_SYSROOT/usr/lib/crtend_android.o"
    fi
}

# This function is used to setup the build environment whenever we
# generate host-specific binaries.
#
setup_host_env ()
{
    CC=$HOST-gcc
    CXX=$HOST-g++
    LD=$HOST-ld
    AR=$HOST-ar
    AS=$HOST-as
    RANLIB=$HOST-ranlib
    NM=$HOST-nm
    STRIP=$HOST-strip
    STRINGS=$HOST-strings
    export CC CXX AS LD AR RANLIB STRIP STRINGS NM

    CFLAGS=
    CXXFLAGS=
    LDFLAGS=
    if [ -z "$NO_STRIP" ]; then
        CFLAGS="-O2 -Os -fomit-frame-pointer -s"
        CXXFLAGS=$CFLAGS
    fi

    # This should only used when building the target GCC libraries
    CFLAGS_FOR_TARGET=$TARGET_CFLAGS
    CXXFLAGS_FOR_TARGET=$TARGET_CXXFLAGS
    LDFLAGS_FOR_TARGET=$TARGET_LDFLAGS

    export CFLAGS CXXFLAGS LDFLAGS CFLAGS_FOR_TARGET CXXFLAGS_FOR_TARGET LDFLAGS_FOR_TARGET

    PATH=$WRAPPERS_DIR:$PATH
}

# $1: NDK architecture name (e.g. 'arm')
arch_sysroot_install_dir ()
{
    echo "$BUILD_DIR/arch-$1/sysroot"
}

# $1: NDK architecture name (e.g. 'arm')
arch_sysroot_dir ()
{
    echo "$(arch_sysroot_install_dir $1)/$(get_default_platform_sysroot_for_arch $1)"
}

# $1: architecture name
gen_minimal_sysroot ()
{
    local ARCH=$1
    local INSTALL_DIR=$(arch_sysroot_install_dir $ARCH)

    dump "[$ARCH] Generating minimal sysroot."
    run2 $NDK_BUILDTOOLS_PATH/gen-platforms.sh --minimal --arch=$ARCH --dst-dir="$INSTALL_DIR"
}


# $1: gmp version
extract_gmp_sources ()
{
    local SRC_DIR="$TOP_BUILD_DIR/temp-src"

    dump "Extracting gmp-$1"
    run2 mkdir -p "$SRC_DIR" &&
    run2 tar xjf "$TOOLCHAIN_SRC_DIR/gmp/gmp-$1.tar.bz2" -C "$SRC_DIR"
}

# $1: gmp version
build_gmp ()
{
    local SRC_DIR="$TOP_BUILD_DIR/temp-src/gmp-$1"
    local INSTALL_DIR="$(host_prereqs_install_dir)"
    local BUILD_DIR

    stamps_do extract-gmp-$1 extract_gmp_sources $1

    dump "[$HOST] Building gmp-$1"
    (
        setup_host_env &&
        BUILD_DIR="$(host_build_dir_for gmp-$GMP_VERSION)" &&
        run2 mkdir -p "$BUILD_DIR" && run2 rm -rf "$BUILD_DIR"/* &&
        cd "$BUILD_DIR" &&
        run2 "$SRC_DIR"/configure \
            --prefix=$INSTALL_DIR \
            --build=$BUILD \
            --host=$HOST \
            --disable-shared &&
        run2 make -j$NUM_JOBS &&
        run2 make install -j$NUM_INSTALL_JOBS
    )
    return $?
}

extract_mpfr_sources ()
{
    local SRC_DIR="$TOP_BUILD_DIR/temp-src"

    dump "Extracting mpfr-$1"
    run2 mkdir -p "$SRC_DIR" &&
    run2 tar xjf "$TOOLCHAIN_SRC_DIR/mpfr/mpfr-$1.tar.bz2" -C "$SRC_DIR"
}

# $1: mpfr-version
build_mpfr ()
{
    local SRC_DIR="$TOP_BUILD_DIR/temp-src/mpfr-$1"
    local INSTALL_DIR="$(host_prereqs_install_dir)"
    local BUILD_DIR

    stamps_do extract-mpfr-$MPFR_VERSION extract_mpfr_sources $1

    stamps_do build-gmp-$GMP_VERSION-$HOST build_gmp $GMP_VERSION

    dump "[$HOST] Building mpfr-$1"
    (
        setup_host_env &&
        BUILD_DIR="$(host_build_dir_for mpfr-$MPFR_VERSION)" &&
        run2 mkdir -p "$BUILD_DIR" && run2 rm -rf "$BUILD_DIR"/* &&
        cd $BUILD_DIR &&
        run2 "$SRC_DIR"/configure \
            --prefix=$INSTALL_DIR \
            --build=$BUILD \
            --host=$HOST \
            --disable-shared \
            --with-gmp=$INSTALL_DIR &&
        run2 make -j$NUM_JOBS &&
        run2 make -j$NUM_INSTALL_JOBS install
    )
    return $?
}

# $1: mpc-version
extract_mpc_sources ()
{
    local SRC_DIR="$TOP_BUILD_DIR/temp-src"

    dump "Extracting mpc-$1"
    run2 mkdir -p "$SRC_DIR" &&
    run2 tar xzf "$TOOLCHAIN_SRC_DIR/mpc/mpc-$1.tar.gz" -C "$SRC_DIR"
}


# $1: mpc-version
build_mpc ()
{
    local SRC_DIR="$TOP_BUILD_DIR/temp-src/mpc-$1"
    local INSTALL_DIR="$(host_prereqs_install_dir)"
    local BUILD_DIR

    stamps_do extract-mpc-$1 extract_mpc_sources $1

    stamps_do build-mpfr-$MPFR_VERSION-$HOST build_mpfr $MPFR_VERSION

    dump "[$HOST] Building mpc-$1"
    (
        setup_host_env &&
        BUILD_DIR="$(host_build_dir_for mpc-$MPC_VERSION)" &&
        run2 mkdir -p "$BUILD_DIR" && run2 rm -rf "$BUILD_DIR"/* &&
        cd $BUILD_DIR &&
        run2 "$SRC_DIR"/configure \
            --prefix=$INSTALL_DIR \
            --build=$BUILD \
            --host=$HOST \
            --disable-shared \
            --with-gmp=$INSTALL_DIR \
            --with-mpfr=$INSTALL_DIR &&
        run2 make -j$NUM_JOBS &&
        run2 make -j$NUM_INSTALL_JOBS install
    )
    return $?
}

# Build all pre-required host libraries (gmp, mpfr, etc...) that are needed
# by binutils and gcc, as static libraries that will be placed under
# $HOST_BUILD_DIR/temp-install
#
# $1: toolchain source directory
#
build_host_prereqs ()
{
    local INSTALL_DIR="$(host_prereqs_install_dir)"
    local ARGS

    ARGS=" --with-gmp=$INSTALL_DIR --with-mpfr=$INSTALL_DIR"

    # Only build MPC when we need it.
    if [ "$HOST_NEED_MPC" ]; then
        ARGS=$ARGS" --with-mpc=$INSTALL_DIR"
        stamps_do build-mpc-$MPC_VERSION-$HOST build_mpc $MPC_VERSION
    else
        stamps_do build-mpfr-$MPFR_VERSION-$HOST build_mpfr $MPFR_VERSION
    fi

    # This gets used by build_host_binutils and others.
    HOST_PREREQS_ARGS=$ARGS
}

build_host_binutils ()
{
    local SRC_DIR="$TOOLCHAIN_SRC_DIR/binutils/binutils-$BINUTILS_VERSION"
    local INSTALL_DIR="$(host_binutils_install_dir)"
    local PREREQS_INSTALL_DIR="$(host_prereqs_install_dir)"
    local ARGS

    build_host_prereqs

    ARGS=" --with-gmp=$PREREQS_INSTALL_DIR --with-mpfr=$PREREQS_INSTALL_DIR"
    if [ "$HOST_NEED_MPC" ]; then
        ARGS=$ARGS" --with-mpc=$PREREQS_INSTALL_DIR"
    fi

    # Enable Gold, for specific builds. The version before binutils 2.21
    # is buggy so don't use it
    case $HOST_OS in
        windows) USE_GOLD=;; # Gold doesn't compile on Windows
        darwin) USE_GOLD=;;  # Building Gold fails with an internal compiler error on Darwin
        *) USE_GOLD=true;;
    esac

    # Special case, gold in binutil-2.21 doesn't build when targetting mips
    if [ "$BINUTILS_VERSION" = "2.21" -a "$TARGET" = "mipsel-linux-android" ]; then
        USE_GOLD=
    fi

    if [ "$USE_GOLD" ]; then
        # The syntax of the --enable-gold option has changed.
        if version_is_greater_than $BINUTILS_VERSION 2.20; then
            ARGS=$ARGS" --enable-gold=default"
        else
            ARGS=$ARGS" --enable-gold=both/gold"
        fi
    fi

    dump "[$HOST][$TARGET] Building binutils-$BINUTILS_VERSION"
    (
        setup_host_env &&
        BUILD_DIR="$(host_build_dir_for binutils-$BINUTILS_VERSION-$TARGET)" &&
        run2 mkdir -p "$BUILD_DIR" && run2 rm -rf "$BUILD_DIR"/* &&
        cd "$BUILD_DIR" &&
        run2 "$SRC_DIR"/configure \
            --prefix="$INSTALL_DIR" \
            --disable-shared \
            --disable-werror \
            --disable-nls \
            --build=$BUILD \
            --host=$HOST \
            --target=$TARGET \
            --with-sysroot="$INSTALL_DIR/sysroot" \
            $ARGS &&
        run2 make -j$NUM_JOBS &&
        run2 make -j$NUM_INSTALL_JOBS install &&
        # We need to take care of something weird, binutils-2.21 on mips
        # doesn't seem to build gold, and the Makefile script forgets to
        # copy it to $INSTALL/bin/mipsel-linux-android-ld. Take care of this
        # here with a symlink, which will be enough for now.
        if [ ! -f "$INSTALL_DIR/bin/$TARGET-ld" ]; then
            run2 ln -s "$TARGET-ld.bfd" "$INSTALL_DIR/bin/$TARGET-ld"
        fi
    )
    return $?
}

copy_target_sysroot ()
{
    local SRC_SYSROOT=$(arch_sysroot_dir $TARGET_ARCH)
    local SYSROOT=$(host_sysroot)

    # We need the arch-specific minimal sysroot
    stamps_do sysroot-arch-$TARGET_ARCH gen_minimal_sysroot $TARGET_ARCH

    dump "[$HOST][$TOOLCHAIN] Copying $TARGET_ARCH sysroot"
    run2 rm -rf "$SYSROOT" &&
    run2 copy_directory "$SRC_SYSROOT" "$SYSROOT"
}

build_host_gcc_core ()
{
    local SRC_DIR="$TOOLCHAIN_SRC_DIR/gcc/gcc-$GCC_VERSION"
    local INSTALL_DIR="$(host_gcc_install_dir)"
    local ARGS NEW_PATH

    stamps_do build-binutils-$BINUTILS_VERSION-$HOST-$TARGET build_host_binutils
    stamps_do sysroot-gcc-$SYSTEM-$TOOLCHAIN copy_target_sysroot

    build_host_prereqs

    NEW_PATH=$(host_gcc_install_dir)/bin:$(host_binutils_install_dir)/bin
    if [ "$HOST" != "$BUILD" ]; then
        NEW_PATH=$(build_gcc_install_dir)/bin:$(build_binutils_install_dir)/bin
    fi

    ARGS=$HOST_PREREQS_ARGS

    ARGS=$ARGS" --with-gnu-as --with-gnu-ld"
    ARGS=$ARGS" --enable-threads --disable-libssp --disable-libmudflap"
    ARGS=$ARGS" --disable-libgomp"  # TODO: Add option to enable this
    ARGS=$ARGS" --disable-libstdc__-v3 --disable-sjlj-exceptions"
    ARGS=$ARGS" --disable-tls"
    ARGS=$ARGS" --disable-libquadmath --disable-plugin --disable-libitm --disable-bootstrap"
    ARGS=$ARGS" --enable-languages=c,c++"
    ARGS=$ARGS" --disable-shared"
    ARGS=$ARGS" --disable-nls"
    ARGS=$ARGS" --disable-werror"
    ARGS=$ARGS" --enable-target-optspace"

    case $TARGET_ARCH in
        arm)
            ARGS=$ARGS" --with-float=soft --with-fpu=vfp --with-arch=armv5te"
            ;;
    esac

    dump "[$HOST][$TOOLCHAIN] Building gcc-core"
    (
        setup_host_env &&
        BUILD_DIR="$(host_build_dir_for gcc-$GCC_VERSION-$TARGET)" &&
        run2 mkdir -p "$BUILD_DIR" && run2 rm -rf "$BUILD_DIR"/* &&
        cd "$BUILD_DIR" &&
        PATH=$NEW_PATH:$PATH &&
        run2 "$SRC_DIR"/configure \
            --prefix="$INSTALL_DIR" \
            --build=$BUILD \
            --host=$HOST \
            --target=$TARGET \
            --with-sysroot="$INSTALL_DIR/sysroot" \
            $HOST_PREREQS_ARGS $ARGS &&
        run2 make -j$NUM_JOBS all-gcc &&
        run2 make -j$NUM_INSTALL_JOBS install-gcc
    )
    return $?
}

build_target_gcc_libs ()
{
    local SRC_DIR="$TOOLCHAIN_SRC_DIR/gcc/gcc-$GCC_VERSION"
    local INSTALL_DIR="$(host_gcc_install_dir)"
    local ARGS NEW_PATH

    stamps_do gcc-core-$GCC_VERSION-$SYSTEM-$TOOLCHAIN build_host_gcc_core

    NEW_PATH=$(host_gcc_install_dir)/bin:$(host_binutils_install_dir)/bin

    dump "[$HOST][$TOOLCHAIN] Building target libraries"
    (
        setup_host_env &&
        BUILD_DIR="$(host_build_dir_for gcc-$GCC_VERSION-$TARGET)" &&
        cd "$BUILD_DIR" &&
        PATH=$NEW_PATH:$PATH &&
        run2 make -j$NUM_JOBS all &&
        run2 make -j$NUM_INSTALL_JOBS install
    )
    return $?
}

copy_target_gcc_libs ()
{
    local SRC_DIR DST_DIR
    dump "[$HOST][$TOOLCHAIN] Copying target GCC libraries"

    SRC_DIR="$(build_gcc_install_dir)/$TARGET"
    DST_DIR="$(host_gcc_install_dir)/$TARGET"

    run2 copy_directory "$SRC_DIR" "$DST_DIR"
}

build_host_gcc ()
{
    if [ "$SYSTEM" = "$BUILD_TAG" -a -z "$NO_TARGET_LIBS" ]; then
        # This is a regular-cross build, and we need to the target GCC libraries.
        stamps_do gcc-all-$GCC_VERSION-$SYSTEM-$TOOLCHAIN build_target_gcc_libs
    else
        # This is a canadian-cross build, or we don't need the target GCC
        # libraries.
        stamps_do gcc-core-$GCC_VERSION-$SYSTEM-$TOOLCHAIN build_host_gcc_core
    fi
}

build_gcc ()
{
    local SYSTEM=$1
    local TOOLCHAIN=$2

    # When performing canadian-cross builds, ensure we generate the
    # host toolchain first (we don't need target GCC libraries though)
    if [ "$SYSTEM" != "$BUILD_TAG" ]; then
        build_gcc $BUILD_TAG $TOOLCHAIN
    fi

    # We do this both in the setup and build phase to ensure we perform
    # as many checks as possible before launching the (long) build procedure.
    setup_build_for_host $SYSTEM
    setup_build_for_toolchain $TOOLCHAIN

    if [ "$PHASE" = build ]; then
        stamps_do build-gcc-$SYSTEM-$TOOLCHAIN build_host_gcc
    fi
}

install_gcc ()
{
    local SYSTEM=$1
    local TOOLCHAIN=$2
    local BINUTILS_DIR GCC_DIR TARGET_LIBS_DIR INSTALL_DIR PROG

    build_gcc $SYSTEM $TOOLCHAIN

    dump "[$HOST][$TOOLCHAIN] Installing to NDK."

    BINUTILS_DIR=$(host_binutils_install_dir)
    GCC_DIR=$(host_gcc_install_dir)
    TARGET_LIBS_DIR=$(build_gcc_install_dir)
    INSTALL_DIR=$TOOLCHAIN_INSTALL_DIR

    # Copy binutils binaries
    run2 copy_directory "$BINUTILS_DIR/bin" "$INSTALL_DIR/bin" &&
    run2 copy_directory "$BINUTILS_DIR/$TARGET/lib" "$INSTALL_DIR/$TARGET/lib" &&

    # Copy gcc core binaries
    run2 copy_directory "$GCC_DIR/bin" "$INSTALL_DIR/bin" &&
    run2 copy_directory "$GCC_DIR/lib/gcc/$TARGET" "$INSTALL_DIR/lib/gcc/$TARGET" &&
    run2 copy_directory "$GCC_DIR/libexec/gcc/$TARGET" "$INSTALL_DIR/libexec/gcc/$TARGET" &&

    # Copy target gcc libraries
    run2 copy_directory "$TARGET_LIBS_DIR/lib/gcc/$TARGET" "$INSTALL_DIR/lib/gcc/$TARGET"

    # We need to generate symlinks for the binutils binaries from
    # $INSTALL_DIR/$TARGET/bin/$PROG to $INSTALL_DIR/bin/$TARGET-$PROG
    mkdir -p "$INSTALL_DIR/$TARGET/bin" &&
    for PROG in $(cd $BINUTILS_DIR/$TARGET/bin && ls *); do
        (cd "$INSTALL_DIR/$TARGET/bin" && rm -f $PROG && ln -s ../../bin/$TARGET-$PROG $PROG)
        fail_panic
    done
}

setup_build

for PHASE in setup build; do
    for SYSTEM in $HOST_SYSTEMS; do
        setup_build_for_host $SYSTEM
        for TOOLCHAIN in $TOOLCHAINS; do
            build_gcc $SYSTEM $TOOLCHAIN
        done
    done
done

for SYSTEM in $HOST_SYSTEMS; do
    setup_build_for_host $SYSTEM
    for TOOLCHAIN in $TOOLCHAINS; do
        install_gcc $SYSTEM $TOOLCHAIN
    done
done