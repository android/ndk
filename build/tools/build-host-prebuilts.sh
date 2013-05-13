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
# Rebuild all host toolchains and programs
#

PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "NDK installation directory"

SYSTEMS=$HOST_TAG
# Replace x86_64 by x86 at the end of SYSTEMS since we will produce
# 32-bit toolchains by default, i.e. unless you use the --try-64 flag
if [ "${SYSTEMS%%x86_64}" != "$SYSTEMS" ]; then
    SYSTEMS=${SYSTEMS%%x86_64}x86
fi
# If we are on Linux, we are able to generate the Windows binaries
# with the mingw32 cross-toolchain.
if [ "$SYSTEMS" = "linux-x86" ]; then
    SYSTEMS=$SYSTEMS",windows"
    # If darwin toolchain exist, build darwin too
    if [ -f "${DARWIN_TOOLCHAIN}-gcc" ]; then
        SYSTEMS=$SYSTEMS",darwin-x86"
    fi
fi
CUSTOM_SYSTEMS=
register_option "--systems=<names>" do_SYSTEMS "List of host systems to build for"
do_SYSTEMS () { CUSTOM_SYSTEMS=true; SYSTEMS=$1; }

ARCHS=$DEFAULT_ARCHS
register_var_option "--arch=<list>" ARCHS "List of target archs to build for"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Package toolchain into this directory"

DARWIN_SSH=
register_var_option "--darwin-ssh=<hostname>" DARWIN_SSH "Generate darwin packages on remote host"

NO_GEN_PLATFORMS=
register_var_option "--no-gen-platforms" NO_GEN_PLATFORMS "Don't generate platforms/ directory, use existing one"

LLVM_VERSION_LIST=$DEFAULT_LLVM_VERSION_LIST
register_var_option "--llvm-version-list=<vers>" LLVM_VERSION_LIST "List of LLVM release versions"

CHECK_FLAG=
do_check_option () { CHECK_FLAG="--check"; }
register_option "--check" do_check_option "Check host prebuilts"

register_try64_option

PROGRAM_PARAMETERS="<toolchain-src-dir>"
PROGRAM_DESCRIPTION=\
"This script can be used to rebuild all the host NDK toolchains at once.
You need to give it the path to the toolchain source directory, as
downloaded by the 'download-toolchain-sources.sh' dev-script."

extract_parameters "$@"

# Check toolchain source path
SRC_DIR="$PARAMETERS"
if [ -z "$SRC_DIR" ]; then
    echo "ERROR: Please provide the path to the toolchain source tree. See --help"
    exit 1
fi

if [ ! -d "$SRC_DIR" ]; then
    echo "ERROR: Not a directory: '$SRC_DIR'"
    exit 1
fi

if [ ! -f "$SRC_DIR/build/configure" -o ! -d "$SRC_DIR/gcc/gcc-$DEFAULT_GCC_VERSION" ]; then
    echo "ERROR: The file $SRC_DIR/build/configure or"
    echo "       the directory $SRC_DIR/gcc/gcc-$DEFAULT_GCC_VERSION does not exist"
    echo "This is not the top of a toolchain tree: $SRC_DIR"
    echo "You must give the path to a copy of the toolchain source directories"
    echo "created by 'download-toolchain-sources.sh."
    exit 1
fi

# Now we can do the build
BUILDTOOLS=$ANDROID_NDK_ROOT/build/tools

if [ -z "$NO_GEN_PLATFORMS" ]; then
    echo "Preparing the build..."
    run $BUILDTOOLS/gen-platforms.sh --minimal --dst-dir=$NDK_DIR --ndk-dir=$NDK_DIR --arch=$(spaces_to_commas $ARCHS)
    fail_panic "Could not generate minimal sysroot!"
else
    if [ ! -d "$NDK_DIR/platforms" ]; then
        echo "ERROR: --no-gen-platforms used but directory missing: $NDK_DIR/platforms"
        exit 1
    fi
fi

SYSTEMS=$(commas_to_spaces $SYSTEMS)
ARCHS=$(commas_to_spaces $ARCHS)
LLVM_VERSION_LIST=$(commas_to_spaces $LLVM_VERSION_LIST)

if [ "$DARWIN_SSH" -a -z "$CUSTOM_SYSTEMS" ]; then
    SYSTEMS=" darwin-x86"
fi

FLAGS=
if [ "$VERBOSE" = "yes" ]; then
    FLAGS=$FLAGS" --verbose"
fi
if [ "$VERBOSE2" = "yes" ]; then
    FLAGS=$FLAGS" --verbose"
fi
if [ "$TRY64" = "yes" ]; then
    FLAGS=$FLAGS" --try-64"
else
    force_32bit_binaries
fi
if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
    FLAGS=$FLAGS" --package-dir=$PACKAGE_DIR"
fi

do_remote_host_build ()
{
    local SYSTEM="$1"
    local ARCH="$2"
    local REMOTE_HOST="$3"

    # 1/ Copy the NDK toolchain build scripts
    # 2/ Copy the toolchain sources/package
    # 3/ Ssh to unpack the build scripts, and run them
    # 4/ Copy back the generated prebuilt binaries
    #
    dump "Preparing remote build on $REMOTE_HOST..."

    # First try to create a remote temp directory on the remote host
    # Do it first so we can fail fast, not after spending time preparing
    # large tarballs.
    dump "Creating remote temp directory"
    TMPREMOTE=/tmp/ndk-$USER/darwin-prebuild
    run ssh $REMOTE_HOST "mkdir -p $TMPREMOTE && rm -rf $TMPREMOTE/*"
    fail_panic "Could not create temporary build directory on $REMOTE_HOST"

    # Now, package all the stuff we're going to send in a temporary
    # directory here on the machine, except toolchain sources
    TMPDARWIN=$NDK_TMPDIR/darwin  # Where we're going to package stuff
    log "Using temporary work directory: $TMPDARWIN"
    mkdir -p "$TMPDARWIN"
    dump "Prepare NDK build scripts"
    copy_directory "$ANDROID_NDK_ROOT/build" "$TMPDARWIN/ndk/build"
    copy_file_list "$ANDROID_NDK_ROOT" "$TMPDARWIN/ndk" sources/android/libthread_db
    copy_file_list "$ANDROID_NDK_ROOT" "$TMPDARWIN/ndk" "$STLPORT_SUBDIR"
    copy_file_list "$ANDROID_NDK_ROOT" "$TMPDARWIN/ndk" "$GABIXX_SUBDIR"
    copy_file_list "$ANDROID_NDK_ROOT" "$TMPDARWIN/ndk" sources/host-tools
    dump "Prepare platforms files"
    copy_directory "$NDK_DIR/platforms" "$TMPDARWIN/ndk/platforms"
    dump "Copying NDK build scripts and platform files to remote..."
    (cd "$TMPDARWIN" && tar czf - ndk) | (ssh $REMOTE_HOST tar xzf - -C $TMPREMOTE)
    fail_panic "Could not copy!"
    rm -rf $TMPDARWIN

    # Copy the toolchain sources
    dump "Copy toolchain sources to remote"
    ssh $REMOTE_HOST mkdir -p $TMPREMOTE/toolchain &&
    (cd "$SRC_DIR" && tar czf - .) | (ssh $REMOTE_HOST tar xzf - -C $TMPREMOTE/toolchain)
    fail_panic "Could not copy toolchain!"

    # Time to run the show :-)
    for ARCH in $(commas_to_spaces $ARCHS); do
        dump "Running remote $ARCH toolchain build..."
        SYSROOT=$TMPREMOTE/ndk/platforms/android-$(get_default_api_level_for_arch $ARCH)/arch-$ARCH
        run ssh $REMOTE_HOST "$TMPREMOTE/ndk/build/tools/build-host-prebuilts.sh $TMPREMOTE/toolchain --package-dir=$TMPREMOTE/packages --arch=$ARCH --ndk-dir=$TMPREMOTE/ndk --no-gen-platforms"
        fail_panic "Could not build prebuilt $ARCH toolchain on Darwin!"
    done
    # Get the results
    dump "Copying back Darwin prebuilt packages..."
    mkdir -p $TMPDARWIN/packages && rm -rf $TMPDARWIN/packages/*
    run scp $REMOTE_HOST:$TMPREMOTE/packages/*-darwin-* $TMPDARWIN/packages
    fail_panic "Could not grab Darwin packages!"

    for PACK in $TMPDARWIN/packages/*; do
        if [ "$PACKAGE_DIR" ]; then
            echo "Copying $(basename $PACK) to $PACKAGE_DIR..."
            cp $PACK $PACKAGE_DIR/
        else
            echo "Unpacking $(basename $PACK) into $NDK_DIR..."
            unpack_archive $PACK $NDK_DIR
        fi
        fail_panic ""
    done

    dump "Cleaning up remote machine..."
    run ssh $REMOTE_HOST rm -rf $TMPREMOTE
}

for SYSTEM in $SYSTEMS; do

    # Add --mingw/--darwin flag
    TOOLCHAIN_FLAGS=$FLAGS
    CANADIAN_BUILD=no
    if [ "$HOST_TAG32" = "linux-x86" ]; then
        case "$SYSTEM" in
            windows)
                TOOLCHAIN_FLAGS=$TOOLCHAIN_FLAGS" --mingw"
                CANADIAN_BUILD=yes
                ;;
            darwin-x86)
                TOOLCHAIN_FLAGS=$TOOLCHAIN_FLAGS" --darwin"
                CANADIAN_BUILD=yes
                ;;
        esac
    fi

    # Should we do a remote build?
    if [ "$SYSTEM" != "$HOST_TAG32" -a "$CANADIAN_BUILD" != "yes" ]; then
        case $SYSTEM in
            darwin-*)
                if [ "$DARWIN_SSH" ]; then
                    do_remote_host_build "$SYSTEM" "$ARCH" "$DARWIN_SSH"
                else
                    echo "WARNING: Can't build $SYSTEM binaries on this system!"
                    echo "Consider using the --darwin-ssh option."
                fi
                continue
        esac
    fi

    # Determin the display system name
    SYSNAME=$SYSTEM
    if [ "$TRY64" = "yes" ]; then
        case $SYSTEM in
            darwin-x86|linux-x86)
                SYSNAME=${SYSTEM%%x86}x86_64
                ;;
            windows)
                SYSNAME=windows-x86_64
                ;;
        esac
    fi

    # First, ndk-stack
    echo "Building $SYSNAME ndk-stack"
    run $BUILDTOOLS/build-ndk-stack.sh $TOOLCHAIN_FLAGS
    fail_panic "ndk-stack build failure!"

    echo "Building $SYSNAME ndk-depends"
    run $BUILDTOOLS/build-ndk-depends.sh $TOOLCHAIN_FLAGS
    fail_panic "ndk-depends build failure!"

    echo "Building $SYSNAME ndk-make"
    run $BUILDTOOLS/build-host-make.sh $TOOLCHAIN_FLAGS
    fail_panic "make build failure!"

    echo "Building $SYSNAME ndk-awk"
    run $BUILDTOOLS/build-host-awk.sh $TOOLCHAIN_FLAGS
    fail_panic "awk build failure!"

    echo "Building $SYSNAME ndk-sed"
    run $BUILDTOOLS/build-host-sed.sh $TOOLCHAIN_FLAGS
    fail_panic "sed build failure!"

    # ToDo: perl in windows/darwin cross.
    MAKE_PERL=no
    case $SYSTEM in
        linux*)
            MAKE_PERL=yes
        ;;
        darwin*)
            # Only works if not cross compiling.
            if [ "$CANADIAN_BUILD" = "no" ] ; then
                MAKE_PERL=yes
            fi
        ;;
        *)
        ;;
    esac

    if [ "$MAKE_PERL" = "yes" ] ; then
        echo "Building $SYSNAME ndk-perl"
        run $BUILDTOOLS/build-host-perl.sh $TOOLCHAIN_FLAGS "$SRC_DIR"
        fail_panic "perl build failure!"
    fi

    echo "Building $SYSNAME ndk-python"
    run $BUILDTOOLS/build-host-python.sh $TOOLCHAIN_FLAGS "--toolchain-src-dir=$SRC_DIR" "--systems=$SYSTEM" "--force"
    fail_panic "python build failure!"

    if [ "$SYSTEM" = "windows" ]; then
        echo "Building $SYSNAME toolbox"
        run $BUILDTOOLS/build-host-toolbox.sh $FLAGS
        fail_panic "Windows toolbox build failure!"
    fi

    # Then the toolchains
    for ARCH in $ARCHS; do
        TOOLCHAIN_NAMES=$(get_toolchain_name_list_for_arch $ARCH)
        if [ -z "$TOOLCHAIN_NAMES" ]; then
            echo "ERROR: Invalid architecture name: $ARCH"
            exit 1
        fi

        for TOOLCHAIN_NAME in $TOOLCHAIN_NAMES; do
            echo "Building $SYSNAME toolchain for $ARCH architecture: $TOOLCHAIN_NAME"
            run $BUILDTOOLS/build-gcc.sh "$SRC_DIR" "$NDK_DIR" $TOOLCHAIN_NAME $TOOLCHAIN_FLAGS --with-python=prebuilt
            fail_panic "Could not build $TOOLCHAIN_NAME-$SYSNAME!"
        done
    done

    # Build llvm and clang
    POLLY_FLAGS=
    if [ "$TRY64" != "yes" -a "$SYSTEM" != "windows" ]; then
        POLLY_FLAGS="--with-polly"
    fi
    for LLVM_VERSION in $LLVM_VERSION_LIST; do
        echo "Building $SYSNAME clang/llvm-$LLVM_VERSION"
        run $BUILDTOOLS/build-llvm.sh "$SRC_DIR" "$NDK_DIR" "llvm-$LLVM_VERSION" $TOOLCHAIN_FLAGS $POLLY_FLAGS $CHECK_FLAG
        fail_panic "Could not build llvm for $SYSNAME"
    done

    # Deploy ld.mcld
    $PROGDIR/deploy-host-mcld.sh --package-dir=$PACKAGE_DIR --systems=$SYSNAME
    fail_panic "Could not deploy ld.mcld for $SYSNAME"

    # We're done for this system
done

# Build tools common to all system
run $BUILDTOOLS/build-analyzer.sh "$SRC_DIR" "$NDK_DIR" "llvm-$DEFAULT_LLVM_VERSION" --package-dir="$PACKAGE_DIR"

if [ "$PACKAGE_DIR" ]; then
    echo "Done, please look at $PACKAGE_DIR"
else
    echo "Done"
fi

exit 0
