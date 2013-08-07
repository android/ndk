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

PROGRAM_PARAMETERS="[<toolchain-name>+]"
PROGRAM_DESCRIPTION="\
This program is used to rebuild one or more NDK cross-toolchains from scratch.
To use it, you will first need to call download-toolchain-sources.sh to download
the toolchain sources, then pass the corresponding directory with the
--toolchain-src-dir=<path> option.

If you don't pass any parameter, the script will rebuild all NDK toolchains
for the current host system [$HOST_TAG]. You can otherwise give a list of
toolchains among the following names:

  arm-linux-androideabi-4.4.3
  arm-linux-androideabi-4.6
  x64-4.4.3
  x86-4.6
  mipsel-linux-android-4.4.3
  mipsel-linux-android-4.6

By default, the script rebuilds the toolchain(s) for you host system [$HOST_TAG],
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
    arm-linux-androideabi-4.4.3 \
    arm-linux-androideabi-4.6

You can build Windows binaries on Linux if you have a Windows-targetting
cross-toolchain installed and in your path. Note that the script named
'build-mingw64-toolchain.sh' can be used to rebuild such a toolchain
(x86_64-w64-mingw32) from sources if you don't have one available.

Building the toolchains directly under Cygwin/MSys has not be tested and
is not recommended (it will be *extremely* slow, if it ever works).

On Darwin, the script will try to probe any compatibility SDK installed
on your development machine and will use the first it finds among a list
of well-known names. You can however force a specific usage with the
--darwin-sdk-version=<version> name, where <version> can be one of 10.4, 10.5,
10.6, 10.7, etc.

The generated binaries should run on 10.5 or higher. You can force a
different compatibility minimum with --darwin-min-version.

If you want to build Darwin binaries on a non-Darwin machine, you will
have to define DARWIN_TOOLCHAIN / DARWIN_SYSROOT in your environment first
(note: this feature is highly experimental).

The script is sufficiently clever to minimize all build steps, especially
if you try to build several toolchains for several distinct host systems. Note
however that generating a canadian-cross toolchain (e.g. building on Linux a
Windows toolchain that targets Android ARM binaries) will force the generation
of a host toolchain as well, in case it is not listed in your --systems list.
This is required to generate proper target GCC libraries.

The toolchain binaries are installed under \$NDK_DIR/toolchains by default,
but you can use --ndk-dir=<path> to specify a different NDK installation path.
The script will try to build the Gold linker for host/target combination that
are well supported (Gold doesn't build / is buggy for some of them). However,
the BFD linker is still the default used by the generated toolchain. You can
change this behaviour with two options:

   --default-ld=<name>  Changes the default toolchain linker.
                        <name> can be one of 'default', 'bfd' and 'gold'.

                        For now, 'default' is an alias for 'bfd', but we plan
                        to map it to 'gold' for some combos in the future, once
                        we're confident it works reliably

   --force-gold-build   Force the build of the Gold linker, even if it is known
                        to fail or generate a buggy linker. Only use this for
                        experimentation (e.g. with your own patched toolchain
                        sources).
"

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

BUILD_DIR=
register_var_option "--build-dir=<path>" BUILD_DIR "Build GCC into directory"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package prebuilt tarballs into directory"

HOST_SYSTEMS="$HOST_TAG"
register_var_option "--systems=<list>" HOST_SYSTEMS "Build binaries for these host systems"

FORCE=
register_var_option "--force" FORCE "Force full rebuild"

NO_TARGET_LIBS=
register_var_option "--no-target-libs" NO_TARGET_LIBS "Don't build gcc target libs."

NO_STRIP=
register_var_option "--no-strip" NO_STRIP "Don't strip generated binaries."

NO_COLOR=
register_var_option "--no-color" NO_COLOR "Don't output colored text."

if [ "$HOST_OS" = darwin ]; then
    DARWIN_SDK_VERSION=
    register_var_option "--darwin-sdk-version=<version>" DARWIN_SDK "Select Darwin SDK version."

    DARWIN_MIN_VERSION=
    register_var_option "--darwin-min-version=<version>" DARWIN_MIN_VERSION "Select minimum OS X version of generated host toolchains."
fi

DEFAULT_LD=
register_var_option "--default-ld=<name>" DEFAULT_LD "Select default linker ('bfd' or 'gold')."

FORCE_GOLD_BUILD=
register_var_option "--force-gold-build" FORCE_GOLD_BUILD "Always try to build Gold (experimental)."

register_jobs_option


extract_parameters "$@"

TOOLCHAINS=$PARAMETERS
if [ -z "$TOOLCHAINS" ]; then
    TOOLCHAINS="arm-linux-androideabi-4.6,x86-4.6,mipsel-linux-android-4.6"
    dump "Auto-config: $TOOLCHAINS"
fi

if [ -z "$TOOLCHAIN_SRC_DIR" ]; then
    panic "Please use --toolchain-src-dir=<path> to select toolchain source directory."
fi

if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR=/tmp/ndk-$USER/build/host-gcc
fi

case $DEFAULT_LD in
    gold|bfd)
      ;;
    "")
      # We always use the default gold linker.
      # bfd is used for some of the older toolchains or archs not supported by gold.
      DEFAULT_LD=gold
      ;;
    *)
      panic "Invalid --default-ld name '$DEFAULT_LD', valid values are: bfd gold"
      ;;
esac

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

# Important note: When identifying a build or host system, there is
# a difference between 'NDK system tags' and "GNU configuration triplet'.
#
# A "system tag" is specific to the NDK and identifies a given host
# system for the toolchain binaries, valid values:
#
#  linux-x86
#  linux-x86_64
#  windows        (historical alias to windows-x86)
#  windows-x86
#  windows-x86_64
#  darwin-x86
#  darwin-x86_64
#
# A GNU configuration triplet identifies a system too, but it used by
# configure scripts, not the NDK. They vary a lot too and some of the
# scripts are *very* picky about the exact values being used.
#
# Typical values that are known to work properly:
#
#    i686-linux-gnu       (Linux x86 system, with GNU libc)
#    x86_64-linux-gnu     (Same, with x86_64 CPU)
#    i586-mingw32msvc     (Windows 32-bits, MSVCRT.DLL)
#    i586-pc-mingw32msvc  (same)
#    i686-w64-mingw32     (same, slightly different sources)
#    x86_64-w64-mingw32   (Windows 64-bits, MSVCRT.DLL)
#    i686-apple-darwin    (OS X / Darwin, x86 CPU)
#    x86_64-apple-darwin  (OS X / Darwin, x86_64 CPU)
#
# A cross-toolchain will typically use the GNU configuration triplet as
# a prefix for all its binaries, but not always. For example, the 'mingw32'
# package on Ubuntu provides a Windows cross-toolchain that uses the
# i586-mingw32msvc prefix, but if you try to use it as a configuration
# triplet when configuring binutils-2.21, the build will fail. You need to
# pass i586-pc-mingw32msvc instead (binutils-2.19 accepts both).
#
# Another issue is that some toolchains need to use additional compiler
# flags to deal with backwards-compatibility SDKs (Darwin) or 32/64 bit
# code generation. Note all build scripts accept the same mix of
# '--with-cflags=...' or 'export CFLAGS' configuration, which makes
# things pretty difficult to manage.
#
# To work-around these issues, the script will generate "wrapper toolchains"
# with the prefix that the configure scripts expects. I.e. small scripts that
# redirect to the correct toolchain, eventually adding hidden extra compiler
# flags. This seems to completely get rid of the problems described above.
#


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

if [ "$NO_COLOR" ]; then
    COLOR_GREEN=
    COLOR_PURPLE=
    COLOR_CYAN=
    COLOR_END=
else
    COLOR_GREEN="\e[32m"
    COLOR_PURPLE="\e[35m"
    COLOR_CYAN="\e[36m"
    COLOR_END="\e[0m"
fi

# Pretty printing with colors!
host_text ()
{
    printf "[${COLOR_GREEN}${HOST}${COLOR_END}]"
}

toolchain_text ()
{
    printf "[${COLOR_PURPLE}${TOOLCHAIN}${COLOR_END}]"
}

target_text ()
{
    printf "[${COLOR_CYAN}${TARGET}${COLOR_END}]"
}

arch_text ()
{
    # Print arch name in cyan
    printf "[${COLOR_CYAN}${ARCH}${COLOR_END}]"
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
# $1: versioned name (e.g. arm-linux-androideabi-4.6)
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

    if [ "$PACKAGE_DIR" ]; then
        mkdir -p "$PACKAGE_DIR"
        fail_panic "Can't create packaging directory: $PACKAGE_DIR"
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

# Check that a given compiler generates code correctly
#
# This is to detect bad/broken toolchains, e.g. amd64-mingw32msvc
# is totally broken on Ubuntu 10.10 and 11.04
#
# $1: compiler
# $2: optional extra flags
#
check_compiler ()
{
    local CC="$1"
    local TMPC=/tmp/build-host-gcc-$USER-$$.c
    local TMPE=${TMPC%%.c}
    local TMPL=$TMPC.log
    local RET
    shift
    cat > $TMPC <<EOF
int main(void) { return 0; }
EOF
    log_n "Checking compiler code generation ($CC)... "
    $CC -o $TMPE $TMPC "$@" >$TMPL 2>&1
    RET=$?
    rm -f $TMPC $TMPE $TMPL
    if [ "$RET" = 0 ]; then
        log "yes"
    else
        log "no"
    fi
    return $RET
}


# $1: toolchain install dir
# $2: toolchain prefix, no trailing dash (e.g. arm-linux-androideabi)
# $3: optional -m32 or -m64.
try_host_fullprefix ()
{
    local PREFIX="$1/bin/$2"
    shift; shift;
    if [ -z "$HOST_FULLPREFIX" ]; then
        local GCC="$PREFIX-gcc"
        if [ -f "$GCC" ]; then
            if check_compiler "$GCC" "$@"; then
                HOST_FULLPREFIX="${GCC%%gcc}"
                dump "$(host_text) Using host gcc: $GCC $@"
            else
                dump "$(host_text) Ignoring broken host gcc: $GCC $@"
            fi
        fi
    fi
}

# $1: host prefix, no trailing slash (e.g. i686-linux-android)
# $2: optional compiler args (should be empty, -m32 or -m64)
try_host_prefix ()
{
    local PREFIX="$1"
    shift
    if [ -z "$HOST_FULLPREFIX" ]; then
        local GCC="$(which $PREFIX-gcc 2>/dev/null)"
        if [ "$GCC" -a -e "$GCC" ]; then
            if check_compiler "$GCC" "$@"; then
                HOST_FULLPREFIX=${GCC%%gcc}
                dump "$(host_text) Using host gcc: ${HOST_FULLPREFIX}gcc $@"
            else
                dump "$(host_text) Ignoring broken host gcc: $GCC $@"
            fi
        fi
    fi
}

# Used to determine the minimum possible Darwin version that a Darwin SDK
# can target. This actually depends from the host architecture.
# $1: Host architecture name
# out: SDK version number (e.g. 10.4 or 10.5)
darwin_arch_to_min_version ()
{
  if [ "$DARWIN_MIN_VERSION" ]; then
    echo "$DARWIN_MIN_VERSION"
  elif [ "$1" = "x86" ]; then
    echo "10.4"
  else
    echo "10.5"
  fi
}

# Use the check for the availability of a compatibility SDK in Darwin
# this can be used to generate binaries compatible with either Tiger or
# Leopard.
#
# $1: SDK root path
# $2: Darwin architecture
check_darwin_sdk ()
{
    if [ -d "$1" -a -z "$HOST_CFLAGS" ] ; then
        local MINVER=$(darwin_arch_to_min_version $2)
        HOST_CFLAGS="-isysroot $1 -mmacosx-version-min=$MINVER -DMAXOSX_DEPLOYEMENT_TARGET=$MINVER"
        HOST_CXXFLAGS=$HOST_CFLAGS
        HOST_LDFLAGS="-syslibroot $1 -mmacosx-version-min=$MINVER"
        dump "Generating $MINVER-compatible binaries."
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
    dump_n "$(host_text) Checking that the compiler generates $BITS-bits code ($@)... "
    $CC -c -DBITS=$BITS -o /dev/null $TMPC $HOST_CFLAGS "$@" > $TMPL 2>&1
    RET=$?
    rm -f $TMPC $TMPL
    if [ "$RET" = 0 ]; then
        dump "yes"
    else
        dump "no"
    fi
    return $RET
}

# This function probes the system to find the best toolchain or cross-toolchain
# to build binaries that run on a given host system. After that, it generates
# a wrapper toolchain under $WRAPPERS_DIR with a prefix of ${HOST}-
# where $HOST is a GNU configuration name.
#
# Important: this script might redefine $HOST to a different value!
# Important: must be called after setup_build.
#
# $1: NDK system tag (e.g. linux-x86)
#
select_toolchain_for_host ()
{
    local HOST_CFLAGS HOST_CXXFLAGS HOST_LDFLAGS HOST_FULLPREFIX DARWIN_ARCH

    # We do all the complex auto-detection magic in the setup phase,
    # then save the result in host-specific global variables.
    #
    # In the build phase, we will simply restore the values into the
    # global HOST_FULLPREFIX / HOST_BUILD_DIR
    # variables.
    #

    # Try to find the best toolchain to do that job, assuming we are in
    # a full Android platform source checkout, we can look at the prebuilts/
    # directory.
    case $1 in
        linux-x86)
            # If possible, automatically use our custom toolchain to generate
            # 32-bit executables that work on Ubuntu 8.04 and higher.
            try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/i686-linux-glibc2.7-4.6" i686-linux
            try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/i686-linux-glibc2.7-4.4.3" i686-linux
            try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilt/linux-x86/toolchain/i686-linux-glibc2.7-4.4.3" i686-linux
            try_host_prefix i686-linux-gnu
            try_host_prefix i686-linux
            try_host_prefix x86_64-linux-gnu -m32
            try_host_prefix x86_64-linux -m32
            ;;

        linux-x86_64)
            # If possible, automaticaly use our custom toolchain to generate
            # 64-bit executables that work on Ubuntu 8.04 and higher.
            try_host_fullprefix "$(dirname $ANDROID_NDK_ROOT)/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.7-4.6" x86_64-linux
            try_host_prefix x86_64-linux-gnu
            try_host_prefix x84_64-linux
            try_host_prefix i686-linux-gnu -m64
            try_host_prefix i686-linux -m64
            ;;

        darwin-*)
            DARWIN_ARCH=$(tag_to_arch $1)
            case $BUILD_OS in
                darwin)
                    if [ "$DARWIN_SDK_VERSION" ]; then
                        # Compute SDK subdirectory name
                        case $DARWIN_SDK_VERSION in
                            10.4) DARWIN_SDK_SUBDIR=$DARWIN_SDK.sdku;;
                            *) DARWIN_SDK_SUBDIR=$DARWIN_SDK.sdk;;
                        esac
                        # Since xCode moved to the App Store the SDKs have been 'sandboxed' into the Xcode.app folder.
                        check_darwin_sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$DARWIN_SDK_SUBDIR $DARWIN_ARCH
                        check_darwin_sdk /Developer/SDKs/MacOSX$DARWIN_SDK_SUBDIR $DARWIN_ARCH
                    else
                        # Since xCode moved to the App Store the SDKs have been 'sandboxed' into the Xcode.app folder.
                        check_darwin_sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk $DARWIN_ARCH
                        check_darwin_sdk /Developer/SDKs/MacOSX10.7.sdk  $DARWIN_ARCH
                        check_darwin_sdk /Developer/SDKs/MacOSX10.6.sdk  $DARWIN_ARCH
                        # NOTE: The 10.5.sdk on Lion is buggy and cannot build basic C++ programs
                        #check_darwin_sdk /Developer/SDKs/MacOSX10.5.sdk  $DARWIN_ARCH
                        # NOTE: The 10.4.sdku is not available anymore and could not be tested.
                        #check_darwin_sdk /Developer/SDKs/MacOSX10.4.sdku $DARWIN_ARCH
                    fi
                    if [ -z "$HOST_CFLAGS" ]; then
                        local version="$(sw_vers -productVersion)"
                        log "Generating $version-compatible binaries!"
                    fi
                    ;;
                *)
                    if [ -z "$DARWIN_TOOLCHAIN" -o -z "$DARWIN_SYSROOT" ]; then
                        dump "If you want to build Darwin binaries on a non-Darwin machine,"
                        dump "Please define DARWIN_TOOLCHAIN to name it, and DARWIN_SYSROOT to point"
                        dump "to the SDK. For example:"
                        dump ""
                        dump "   DARWIN_TOOLCHAIN=\"i686-apple-darwin11\""
                        dump "   DARWIN_SYSROOT=\"~/darwin-cross/MacOSX10.7.sdk\""
                        dump "   export DARWIN_TOOLCHAIN DARWIN_SYSROOT"
                        dump ""
                        exit 1
                    fi
                    local DARWINMINVER=$(darwin_arch_to_min_version $2)
                    check_darwin_sdk $DARWIN_SYSROOT $DARWINARCH
                    try_host_prefix "$DARWIN_TOOLCHAIN" -m$(tag_to_bits $1) --sysroot "$DARWIN_SYSROOT"
                    if [ -z "$HOST_FULLPREFIX" ]; then
                        dump "It looks like $DARWIN_TOOLCHAIN-gcc is not in your path, or does not work correctly!"
                        exit 1
                    fi
                    dump "Using darwin cross-toolchain: ${HOST_FULLPREFIX}gcc"
                    ;;
            esac
            ;;

        windows|windows-x86)
            case $BUILD_OS in
                linux)
                    # We favor these because they are more recent, and because
                    # we have a script to rebuild them from scratch. See
                    # build-mingw64-toolchain.sh.
                    try_host_prefix x86_64-w64-mingw32 -m32
                    try_host_prefix i686-w64-mingw32
                    # Typically provided by the 'mingw32' package on Debian
                    # and Ubuntu systems.
                    try_host_prefix i586-mingw32msvc
                    # Special note for Fedora: this distribution used
                    # to have a mingw32-gcc package that provided a 32-bit
                    # only cross-toolchain named i686-pc-mingw32.
                    # Later versions of the distro now provide a new package
                    # named mingw-gcc which provides i686-w64-mingw32 and
                    # x86_64-w64-mingw32 instead.
                    try_host_prefix i686-pc-mingw32
                    if [ -z "$HOST_FULLPREFIX" ]; then
                        dump "There is no Windows cross-compiler. Ensure that you"
                        dump "have one of these installed and in your path:"
                        dump "   x86_64-w64-mingw32-gcc  (see build-mingw64-toolchain.sh)"
                        dump "   i686-w64-mingw32-gcc    (see build-mingw64-toolchain.sh)"
                        dump "   i586-mingw32msvc-gcc    ('mingw32' Debian/Ubuntu package)"
                        dump "   i686-pc-mingw32         (on Fedora)"
                        dump ""
                        exit 1
                    fi
                    # Adjust $HOST to match the toolchain to ensure proper builds.
                    # I.e. chose configuration triplets that are known to work
                    # with the gmp/mpfr/mpc/binutils/gcc configure scripts.
                    case $HOST_FULLPREFIX in
                        *-mingw32msvc-*|i686-pc-mingw32)
                            HOST=i586-pc-mingw32msvc
                            ;;
                        *)
                            HOST=i686-w64-mingw32msvc
                            ;;
                    esac
                    ;;
                *) panic "Sorry, this script only supports building windows binaries on Linux."
                ;;
            esac
            ;;

        windows-x86_64)
            # Sanity check for GMP which doesn't build with x86_64-w64-mingw32-gcc
            # before 5.0. We already have 5.0.5 in AOSP toolchain source tree, so
            # suggest it here.
            if ! version_is_greater_than $GMP_VERSION 5.0; then
                dump "You cannot build a 64-bit Windows toolchain with this version of libgmp."
                dump "Please use --gmp-version=5.0.5 to fix this."
                exit 1
            fi
            case $BUILD_OS in
                linux)
                    # See comments above for windows-x86
                    try_host_prefix x86_64-w64-mingw32
                    try_host_prefix i686-w64-mingw32 -m64
                    # Beware that this package is completely broken on many
                    # versions of no vinegar Ubuntu (i.e. it fails at building trivial
                    # programs).
                    try_host_prefix amd64-mingw32msvc
                    # There is no x86_64-pc-mingw32 toolchain on Fedora.
                    if [ -z "$HOST_FULLPREFIX" ]; then
                        dump "There is no Windows cross-compiler in your path. Ensure you"
                        dump "have one of these installed and in your path:"
                        dump "   x86_64-w64-mingw32-gcc  (see build-mingw64-toolchain.sh)"
                        dump "   i686-w64-mingw32-gcc    (see build-mingw64-toolchain.sh)"
                        dump "   amd64-mingw32msvc-gcc   (Debian/Ubuntu - broken until Ubuntu 11.10)"
                        dump ""
                        exit 1
                    fi
                    # See comment above for windows-x86
                    case $HOST_FULLPREFIX in
                        *-mingw32msvc*)
                            # Actually, this has never been tested.
                            HOST=amd64-pc-mingw32msvc
                            ;;
                        *)
                            HOST=x86_64-w64-mingw32
                            ;;
                    esac
                    ;;

                *) panic "Sorry, this script only supports building windows binaries on Linux."
                ;;
            esac
            ;;
    esac

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
        fi
        HOST_CFLAGS=$HOST_CFLAGS" "$TRY_CFLAGS
        HOST_CXXFLAGS=$HOST_CXXFLAGS" "$TRY_CFLAGS
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
    local HOST_VARNAME=$(dashes_to_underscores $1)
    local HOST_VAR=_HOST_${HOST_VARNAME}

    # Determine the host configuration triplet in $HOST
    HOST=$(tag_to_config_triplet $1)
    HOST_OS=$(tag_to_os $1)
    HOST_ARCH=$(tag_to_arch $1)
    HOST_BITS=$(tag_to_bits $1)
    HOST_TAG=$1

    # Note: since select_toolchain_for_host can change the value of $HOST
    # we need to save it in a variable to later get the correct one when
    # this function is called again.
    if [ -z "$(var_value ${HOST_VAR}_SETUP)" ]; then
        select_toolchain_for_host $1
        var_assign ${HOST_VAR}_CONFIG $HOST
        var_assign ${HOST_VAR}_SETUP true
    else
        HOST=$(var_value ${HOST_VAR}_CONFIG)
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
        x86_64) TARGET=x86_64-linux-android;;
        mips|mipsel) TARGET=mipsel-linux-android; TARGET_ARCH=mips;;
        *) panic "Unknown target toolchain architecture: $TARGET_ARCH"
    esac

    # MPC is only needed starting with GCC 4.5
    HOST_NEED_MPC=
    if version_is_greater_than $GCC_VERSION 4.5; then
        HOST_NEED_MPC=true
    fi

    # TODO: We will need to place these under
    #      $NDK_DIR/prebuilts/$HOST/android-$TARGET_ARCH-gcc-$GCC_VERSION/
    #      in a future patch.
    TOOLCHAIN_SUB_DIR=toolchains/$TOOLCHAIN/prebuilt/$HOST_TAG
    TOOLCHAIN_INSTALL_DIR=$NDK_DIR/$TOOLCHAIN_SUB_DIR

    # These will go into CFLAGS_FOR_TARGET and others during the build
    # of GCC target libraries.
    if [ -z "$NO_STRIP" ]; then
        TARGET_CFLAGS="-O2 -s"
    else
        TARGET_CFLAGS="-Os -g"
    fi

    TARGET_CXXFLAGS=$TARGET_CFLAGS
    TARGET_LDFLAGS=""

    case $TARGET_ARCH in
        mips)
        # Enable C++ exceptions, RTTI and GNU libstdc++ at the same time
        # You can't really build these separately at the moment.
        # Add -fpic, because MIPS NDK will need to link .a into .so.
        TARGET_CFLAGS=$TARGET_CFLAGS" -fexceptions -fpic"
        TARGET_CXXFLAGS=$TARGET_CXXFLAGS" -frtti -fpic"
        ;;
    esac
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

    dump "$(arch_text) Generating minimal sysroot."
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

    dump "$(host_text) Building gmp-$1"
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

    dump "$(host_text) Building mpfr-$1"
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

    dump "$(host_text) Building mpc-$1"
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

    LD_NAME=$DEFAULT_LD

    # Enable Gold globally. It can be built for all hosts.
    BUILD_GOLD=true

    # Special case, gold is not ready for mips yet.
    if [ "$TARGET" = "mipsel-linux-android" ]; then
        BUILD_GOLD=
    fi

    # Another special case, gold in binutils-2.21 for arch-x86 is buggy
    # (i.e. when building the platform with it, the system doesn't boot)
    #
    if [ "$BINUTILS_VERSION" = "2.21" -a "$TARGET" = "i686-linux-android" ]; then
        USE_LD_DEFAULT=true
        BUILD_GOLD=
    fi

    # Another special case, for arch-x86_64 gold supports x32 starting from 2.23
    #
    if [ "$TARGET" = "x86_64-linux-android" ]; then
       if ! version_is_greater_than $BINUTILS_VERSION 2.23; then
        USE_LD_DEFAULT=true
        BUILD_GOLD=
       fi
    fi

    # Another special case. Not or 2.19, it wasn't ready
    if [ "$BINUTILS_VERSION" = "2.19" ]; then
        BUILD_GOLD=
    fi

    if [ "$DEFAULT_LD" = "gold" -a -z "$BUILD_GOLD" ]; then
        dump "$(host_text)$(target_text): Cannot build Gold for this toolchain!"
        BUILD_GOLD=
    fi

    # Ok, if the user *really* wants it, we're going to build Gold anyway.
    # There are no guarantees about the correctness of the resulting binary.
    # --default-ld still determines the default linker to use by the toolchain.
    #
    if [ "$FORCE_GOLD_BUILD" -a -z "$BUILD_GOLD" ]; then
        dump "$(host_text)$(target_text): Warning: forcing build of potentially buggy Gold linker!"
        BUILD_GOLD=true
    fi

    # The BFD linker is always built, but to build Gold, we need a specific
    # option for the binutils configure script. Note that its format has
    # changed during development.
    export host_configargs=
    if [ "$BUILD_GOLD" ]; then
        # The syntax of the --enable-gold option has changed.
        if version_is_greater_than $BINUTILS_VERSION 2.20; then
            if [ "$DEFAULT_LD" = "bfd" ]; then
                ARGS=$ARGS" --enable-gold --enable-ld=default"
            else
                ARGS=$ARGS" --enable-gold=default --enable-ld"
            fi
        else
            if [ "$DEFAULT_LD" = "bfd" ]; then
                ARGS=$ARGS" --enable-gold=both"
            else
                ARGS=$ARGS" --enable-gold=both/gold"
            fi
        fi
	# This ARG needs quoting when passed to run2.
	GOLD_LDFLAGS_ARG=
        if [ "$HOST_OS" = 'windows' ]; then
            # gold may have runtime dependency on libgcc_sjlj_1.dll and
            # libstdc++-6.dll when built by newer versions of mingw.
            # Link them statically to avoid that.
            if version_is_greater_than $BINUTILS_VERSION 2.22; then
                export host_configargs="--with-gold-ldflags='-static-libgcc -static-libstdc++'"
            elif version_is_greater_than $BINUTILS_VERSION 2.21; then
                GOLD_LDFLAGS_ARG="--with-gold-ldflags=-static-libgcc -static-libstdc++"
            else
                export LDFLAGS=$LDFLAGS" -static-libgcc -static-libstdc++"
            fi
        fi
    fi

    # This is used to install libbfd which is later used to compile
    # oprofile for the platform. This is not technically required for
    # the NDK, but allows us to use the same toolchain for the platform
    # build. TODO: Probably want to move this step to its own script
    # like build-host-libbfd.sh in the future.
    ARGS=$ARGS" --enable-install-libbfd"

    # Enable plugins support for binutils-2.21+
    # This is common feature for binutils and gcc
    case "$BINUTILS_VERSION" in
      2.19)
        # Add nothing
        ;;
      *)
        ARGS=$ARGS" --enable-plugins"
        ;;
    esac

    dump "$(host_text)$(target_text) Building binutils-$BINUTILS_VERSION"
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

    dump "$(host_text)$(toolchain_text) Copying $TARGET_ARCH sysroot"
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

    case "$GCC_VERSION" in
      4.4.3)
        ARGS=$ARGS" --disable-plugin"
        ;;
    esac

    ARGS=$ARGS" --with-gnu-as --with-gnu-ld"
    ARGS=$ARGS" --enable-threads --disable-libssp --disable-libmudflap"
    ARGS=$ARGS" --disable-libstdc__-v3 --disable-sjlj-exceptions"
    ARGS=$ARGS" --disable-tls"
    ARGS=$ARGS" --disable-libquadmath --disable-libitm --disable-bootstrap"
    ARGS=$ARGS" --enable-languages=c,c++"
    ARGS=$ARGS" --disable-shared"
    ARGS=$ARGS" --disable-nls"
    ARGS=$ARGS" --disable-werror"
    ARGS=$ARGS" --enable-target-optspace"
    ARGS=$ARGS" --enable-eh-frame-hdr-for-static"
    # TODO: Build fails for libsanitizer which appears in 4.8. Disable for now.
    ARGS=$ARGS" --disable-libsanitizer"

    case "$GCC_VERSION" in
     4.4.3)
       ARGS=$ARGS" --disable-libgomp"
       ;;
     *)
       case $TARGET_ARCH in
	     arm) ARGS=$ARGS" --enable-libgomp";;
	     x86*) ARGS=$ARGS" --enable-libgomp";;
	     mips|mipsel) ARGS=$ARGS" --disable-libgomp";;
	 esac
	 ;;
    esac

    # Place constructors/destructors in .init_array/.fini_array, not in
    # .ctors/.dtors on Android. Note that upstream Linux GLibc is now doing
    # the same.
    ARGS=$ARGS" --enable-initfini-array"

    case $TARGET_ARCH in
        arm)
            ARGS=$ARGS" --with-arch=armv5te --with-float=soft --with-fpu=vfpv3-d16"
            ;;
        x86)
            ARGS=$ARGS" --with-arch=i686 --with-tune=atom --with-fpmath=sse"
            ;;
        x86_64)
            ARGS=$ARGS" --with-arch=x86-64 --with-tune=atom --with-fpmath=sse --with-multilib-list=m32,m64,mx32"
            ;;
        mips)
            # Add --disable-fixed-point to disable fixed-point support
            # Add --disable-threads for eh_frame handling in a single thread
            ARGS=$ARGS" --with-arch=mips32 --disable-fixed-point --disable-threads"
            ;;
    esac

    dump "$(host_text)$(toolchain_text) Building gcc-core"
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

    dump "$(host_text)$(toolchain_text) Building target libraries"
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
    dump "$(host_text)$(toolchain_text) Copying target GCC libraries"

    SRC_DIR="$(build_gcc_install_dir)/$TARGET"
    DST_DIR="$(host_gcc_install_dir)/$TARGET"

    run2 copy_directory "$SRC_DIR" "$DST_DIR"
}

build_host_gcc ()
{
    if [ "$SYSTEM" = "$BUILD_TAG" -a -z "$NO_TARGET_LIBS" ]; then
        # This is a regular-cross build, and we need to build the target GCC libraries.
        stamps_do gcc-all-$GCC_VERSION-$SYSTEM-$TOOLCHAIN build_target_gcc_libs
    else
        # This is a canadian-cross build, or we don't need the target GCC libraries.
        stamps_do gcc-core-$GCC_VERSION-$SYSTEM-$TOOLCHAIN build_host_gcc_core
    fi
}

# $1: host system tag (e.g. linux-x86)
# $2: toolchain name (e.g. x86-4.4.3)
build_gcc ()
{
    local SYSTEM=$1
    local TOOLCHAIN=$2

    # When performing canadian-cross builds, ensure we generate the
    # host toolchain first (even if we don't need target GCC libraries)
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

do_relink ()
{
    log "Relink $1 --> $2"
    local BASENAME DIRNAME
    DIRNAME=$(dirname "$1")
    BASENAME=$(basename "$1")
    ( cd "$DIRNAME" && rm -f "$BASENAME" && ln -s "$2" "$BASENAME" )
}

# $1: host system tag (e.g. linux-x86)
# $2: toolchain name (e.g. x86-4.4.3)
install_gcc ()
{
    local SYSTEM=$1
    local TOOLCHAIN=$2
    local BINUTILS_DIR GCC_DIR TARGET_LIBS_DIR INSTALL_DIR PROG

    build_gcc $SYSTEM $TOOLCHAIN

    dump "$(host_text)$(toolchain_text) Installing to NDK."

    BINUTILS_DIR=$(host_binutils_install_dir)
    GCC_DIR=$(host_gcc_install_dir)
    TARGET_LIBS_DIR=$(build_gcc_install_dir)
    INSTALL_DIR=$TOOLCHAIN_INSTALL_DIR

    # Copy binutils binaries
    run2 copy_directory "$BINUTILS_DIR/bin" "$INSTALL_DIR/bin" &&
    run2 copy_directory "$BINUTILS_DIR/$TARGET/lib" "$INSTALL_DIR/$TARGET/lib" &&

    # The following is used to copy the libbfd. See --enable-install-libbfd
    # which is set in build_host_binutils above.
    run2 copy_directory "$BINUTILS_DIR/$HOST/$TARGET/include" "$INSTALL_DIR/include" &&
    run2 copy_directory "$BINUTILS_DIR/$HOST/$TARGET/lib"     "$INSTALL_DIR/lib$(tag_to_bits $SYSTEM)" &&

    # Copy gcc core binaries
    run2 copy_directory "$GCC_DIR/bin" "$INSTALL_DIR/bin" &&
    run2 copy_directory "$GCC_DIR/lib/gcc/$TARGET" "$INSTALL_DIR/lib/gcc/$TARGET" &&
    run2 copy_directory "$GCC_DIR/libexec/gcc/$TARGET" "$INSTALL_DIR/libexec/gcc/$TARGET" &&

    # Copy target gcc libraries
    run2 copy_directory "$TARGET_LIBS_DIR/lib/gcc/$TARGET" "$INSTALL_DIR/lib/gcc/$TARGET"
    run2 copy_directory "$TARGET_LIBS_DIR/$TARGET/lib" "$INSTALL_DIR/$TARGET/lib"
    # Multilib compiler should have these
    if [ -d "$TARGET_LIBS_DIR/$TARGET/libx32" ]; then
       run2 copy_directory "$TARGET_LIBS_DIR/$TARGET/libx32" "$INSTALL_DIR/$TARGET/libx32"
    fi
    if [ -d "$TARGET_LIBS_DIR/$TARGET/lib64" ]; then
       run2 copy_directory "$TARGET_LIBS_DIR/$TARGET/lib64" "$INSTALL_DIR/$TARGET/lib64"
    fi

    # We need to generate symlinks for the binutils binaries from
    # $INSTALL_DIR/$TARGET/bin/$PROG to $INSTALL_DIR/bin/$TARGET-$PROG
    mkdir -p "$INSTALL_DIR/$TARGET/bin" &&
    for PROG in $(cd $INSTALL_DIR/$TARGET/bin && ls * 2>/dev/null); do
        do_relink "$INSTALL_DIR/$TARGET/bin/$PROG" ../../bin/$TARGET-$PROG
        fail_panic
    done

    # Also relink a few files under $INSTALL_DIR/bin/
    do_relink "$INSTALL_DIR"/bin/$TARGET-c++ $TARGET-g++ &&
    do_relink "$INSTALL_DIR"/bin/$TARGET-gcc-$GCC_VERSION $TARGET-gcc &&
    if [ -f "$INSTALL_DIR"/bin/$TARGET-ld.gold ]; then
      do_relink "$INSTALL_DIR"/bin/$TARGET-ld $TARGET-ld.gold
    else
      do_relink "$INSTALL_DIR"/bin/$TARGET-ld $TARGET-ld.bfd
    fi
    fail_panic

    # Remove unwanted $TARGET-run simulator to save about 800 KB.
    run2 rm -f "$INSTALL_DIR"/bin/$TARGET-run

    # Copy the license files
    local TOOLCHAIN_LICENSES="$ANDROID_NDK_ROOT"/build/tools/toolchain-licenses
    run cp -f "$TOOLCHAIN_LICENSES"/COPYING "$TOOLCHAIN_LICENSES"/COPYING.LIB "$INSTALL_DIR"
}

# $1: host system tag (e.g. linux-x86)
# $2: toolchain name (e.g. x86-4.4.3)
# $3: package directory.
package_gcc ()
{
    local SYSTEM=$1
    local TOOLCHAIN=$2
    local PACKAGE_DIR="$3"
    local PACKAGE_NAME="$TOOLCHAIN-$SYSTEM.tar.bz2"
    local PACKAGE_FILE="$PACKAGE_DIR/$PACKAGE_NAME"

    setup_build_for_toolchain $TOOLCHAIN

    dump "Packaging $PACKAGE_NAME."
    pack_archive "$PACKAGE_FILE" "$NDK_DIR" "$TOOLCHAIN_SUB_DIR"
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

if [ "$PACKAGE_DIR" ]; then
    for SYSTEM in $HOST_SYSTEMS; do
        setup_build_for_host $SYSTEM
        for TOOLCHAIN in $TOOLCHAINS; do
            package_gcc $SYSTEM $TOOLCHAIN "$PACKAGE_DIR"
        done
    done
    echo "Done. See the content of $PACKAGE_DIR:"
    ls -l "$PACKAGE_DIR"
    echo ""
fi
