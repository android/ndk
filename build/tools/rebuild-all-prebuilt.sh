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

# Download and rebuild all prebuilt binaries for the Android NDK
# This includes:
#    - cross toolchains (gcc, ld, gdb, etc...)
#    - target-specific gdbserver
#    - host ccache
#

. `dirname $0`/prebuilt-common.sh
PROGDIR=`dirname $0`

NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "Don't package, copy binaries to target NDK directory"

BUILD_DIR=/tmp/ndk-$USER/build
register_var_option "--build-dir=<path>" BUILD_DIR "Specify temporary build directory"

GDB_VERSION=$DEFAULT_GDB_VERSION
register_var_option "--gdb-version=<version>" GDB_VERSION "Specify gdb version"

BINUTILS_VERSION=$DEFAULT_BINUTILS_VERSION
register_var_option "--binutils-version=<version>" BINUTILS_VERSION "Specify binutils version"

MPFR_VERSION=$DEFAULT_MPFR_VERSION
register_var_option "--mpfr-version=<version>" MPFR_VERSION "Specify mpfr version"

ARCH=arm
register_var_option "--arch=<arch>" ARCH "Specify architecture"

OPTION_SYSROOT=
register_var_option "--sysroot=<dir>" OPTION_SYSROOT "Specify sysroot"

OPTION_TOOLCHAIN_SRC_PKG=
register_var_option "--toolchain-src-pkg=<file>" OPTION_TOOLCHAIN_SRC_PKG "Use toolchain source package."

OPTION_TOOLCHAIN_SRC_DIR=
register_var_option "--toolchain-src-dir=<path>" OPTION_TOOLCHAIN_SRC_DIR "Use toolchain source directory."

RELEASE=`date +%Y%m%d`
PACKAGE_DIR=/tmp/ndk-$USER/prebuilt-$RELEASE
register_var_option "--package-dir=<path>" PACKAGE_DIR "Put prebuilt tarballs into <path>."

OPTION_GIT_HTTP=no
register_var_option "--git-http" OPTION_GIT_HTTP "Download sources with http."

HOST_ONLY=
register_var_option "--host-only" HOST_ONLY "Only rebuild host binaries."

DARWIN_SSH=
if [ "$HOST_OS" = "linux" ] ; then
register_var_option "--darwin-ssh=<hostname>" DARWIN_SSH "Specify Darwin hostname to ssh to for the build."
fi

register_mingw_option

PROGRAM_PARAMETERS=
PROGRAM_DESCRIPTION=\
"Download and rebuild from scratch all prebuilt binaries for the Android NDK.
By default, this will create prebuilt binary tarballs in: $PACKAGE_DIR

You can however use --ndk-dir=<path> to instead copy the binaries directly
to an existing NDK installation.

By default, the script will download the toolchain sources from the Internet,
but you can override this using either --toolchain-src-dir or
--toolchain-src-pkg.

Please read docs/DEVELOPMENT.TXT for more usage information about this
script.
"

extract_parameters "$@"

# Needed to set HOST_TAG to windows if --mingw is used.
handle_host

if [ -n "$PACKAGE_DIR" -a -n "$NDK_DIR" ] ; then
    echo "ERROR: You cannot use both --package-dir and --ndk-dir at the same time!"
    exit 1
fi

if [ -z "$NDK_DIR" ] ; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create directory: $PACKAGE_DIR"
    NDK_DIR=$BUILD_DIR/install
    mkdir -p $NDK_DIR
else
    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: NDK directory does not exists: $NDK_DIR"
        exit 1
    fi
    PACKAGE_DIR=
fi

if [ -n "$PARAMETERS" ]; then
    dump "ERROR: Too many parameters. See --help for proper usage."
    exit 1
fi

mkdir -p $BUILD_DIR
if [ $? != 0 ] ; then
    dump "ERROR: Could not create build directory: $BUILD_DIR"
    exit 1
fi
setup_default_log_file "$BUILD_DIR/log.txt"

if [ -n "$OPTION_TOOLCHAIN_SRC_DIR" ] ; then
    if [ -n "$OPTION_TOOLCHAIN_SRC_PKG" ] ; then
        dump "ERROR: You can't use both --toolchain-src-dir and --toolchain-src-pkg"
        exi t1
    fi
    SRC_DIR="$OPTION_TOOLCHAIN_SRC_DIR"
    if [ ! -d "$SRC_DIR/gcc" ] ; then
        dump "ERROR: Invalid toolchain source directory: $SRC_DIR"
        exit 1
    fi
else
    SRC_DIR="$BUILD_DIR/src"
    mkdir -p "$SRC_DIR"
fi

FLAGS=""
if [ $VERBOSE = yes ] ; then
    FLAGS="--verbose"
fi
if [ "$MINGW" = "yes" ] ; then
    FLAGS="$FLAGS --mingw"
fi

# If --toolchain-src-dir is not given, get the toolchain sources, either
# by downloading, or by extracting the, from a tarball given by the
# --toolchain-src-pkg option.
#
get_toolchain_sources ()
{
    if [ -z "$OPTION_TOOLCHAIN_SRC_DIR" ] ; then
        if [ -n "$OPTION_TOOLCHAIN_SRC_PKG" ] ; then
            # Unpack the toolchain sources
            if [ ! -f "$OPTION_TOOLCHAIN_SRC_PKG" ] ; then
                dump "ERROR: Invalid toolchain source package: $OPTION_TOOLCHAIN_SRC_PKG"
                exit 1
            fi
            unpack_archive "$OPTION_TOOLCHAIN_SRC_PKG" "$SRC_DIR"
            fail_panic "Could not unpack toolchain sources!"
        else
            # Download the toolchain sources
            dump "Download sources from android.git.kernel.org"
            DOWNLOAD_FLAGS="$FLAGS"
            if [ $OPTION_GIT_HTTP = "yes" ] ; then
                DOWNLOAD_FLAGS="$DOWNLOAD_FLAGS --git-http"
            fi
            $PROGDIR/download-toolchain-sources.sh $DOWNLOAD_FLAGS $SRC_DIR
            fail_panic "Could not download toolchain sources!"
        fi
    fi # ! $TOOLCHAIN_SRC_DIR
}



# Special treatment when building remotely on a Darwin machine through ssh
# For now, we will have to prompt the user several times.
#
if [ -n "$DARWIN_SSH" ] ; then
    # 1/ Copy the NDK toolchain build scripts
    # 2/ Copy the toolchain sources/package
    # 3/ Ssh to unpack the build scripts, and run them
    # 4/ Copy back the generated prebuilt binaries
    #
    dump "Preparing remote build on $DARWIN_SSH..."
    dump "Getting toolchain sources"
    get_toolchain_sources
    dump "Creating remote temp directory"
    TMPREMOTE=/tmp/ndk-$USER/darwin-prebuild
    run ssh $DARWIN_SSH "mkdir -p $TMPREMOTE && rm -rf $TMPREMOTE/*"
    TMPDARWIN=$NDK_TMPDIR/darwin  # Where we're going to package stuff
    log "Using temporary work directory: $TMPDARWIN"
    dump "Prepare NDK build scripts"
    copy_directory "$ANDROID_NDK_ROOT/build" "$TMPDARWIN/ndk/build"
    copy_file_list "$ANDROID_NDK_ROOT" "$TMPDARWIN/ndk" sources/android/libthread_db
    copy_file_list "$ANDROID_NDK_ROOT" "$TMPDARWIN/ndk" "$STLPORT_SUBDIR"
    copy_file_list "$ANDROID_NDK_ROOT" "$TMPDARWIN/ndk" "$GABIXX_SUBDIR"
    copy_file_list "$ANDROID_NDK_ROOT" "$TMPDARWIN/ndk" sources/host-tools/ndk-stack
    dump "Prepare platforms files"
    $PROGDIR/build-platforms.sh --arch=$ARCH --no-samples --dst-dir="$TMPDARWIN/ndk"
    dump "Copying NDK build scripts and platform files to remote..."
    (cd "$TMPDARWIN" && tar czf - ndk) | (ssh $DARWIN_SSH tar xzf - -C $TMPREMOTE)
    fail_panic "Could not copy!"
    rm -rf $TMPDARWIN
    dump "Copy toolchain sources to remote"
    ssh $DARWIN_SSH mkdir -p $TMPREMOTE/toolchain &&
    (cd "$SRC_DIR" && tar czf - .) | (ssh $DARWIN_SSH tar xzf - -C $TMPREMOTE/toolchain)
    fail_panic "Could not copy toolchain!"
    dump "Running remote build..."
    SYSROOT=$TMPREMOTE/ndk/platforms/android-$(get_default_api_level_for_arch $ARCH)/arch-$ARCH
    run ssh $DARWIN_SSH "$TMPREMOTE/ndk/build/tools/rebuild-all-prebuilt.sh --toolchain-src-dir=$TMPREMOTE/toolchain --package-dir=$TMPREMOTE/packages --arch=$(spaces_to_commas $ARCH) --sysroot=$SYSROOT --host-only"
    fail_panic "Could not build prebuilt packages on Darwin!"
    dump "Copying back Darwin prebuilt packages..."
    run scp $DARWIN_SSH:$TMPREMOTE/packages/*-darwin-* $PACKAGE_DIR/
    fail_panic "Could not grab Darwin packages!"
    dump "Cleaning up remote machine..."
    run ssh $DARWIN_SSH rm -rf $TMPREMOTE
    exit 0
fi

# Package a directory in a .tar.bz2 archive
#
# $1: textual description
# $2: final package name (without .tar.bz2 suffix)
# $3: relative root path from $NDK_DIR
#
package_it ()
{
    if [ -n "$PACKAGE_DIR" ] ; then
        dump "Packaging $1 ($2.tar.bz2) ..."
        PREBUILT_PACKAGE="$PACKAGE_DIR/$2".tar.bz2
        (cd $NDK_DIR && tar cjf $PREBUILT_PACKAGE $3)
        fail_panic "Could not package $1!"
    fi
}


# Rebuild the ndk-stack tool, and link to it
dump "Building host ndk-stack tool from sources."
NDK_STACK_FLAGS=
NDK_STACK_PACKAGE=$(get_host_exec_name ndk-stack)
if [ "$MINGW" = yes ]; then
    NDK_STACK_FLAGS=$NDK_STACK_FLAGS" --mingw"
fi
if [ "$VERBOSE" = "yes" ]; then
    NDK_STACK_FLAGS=$NDK_STACK_FLAGS" --verbose"
fi
if [ "$VERBOSE2" = "yes" ]; then
    NDK_STACK_FLAGS=$NDK_STACK_FLAGS" --verbose"
fi
$ANDROID_NDK_ROOT/build/tools/build-ndk-stack.sh $NDK_STACK_FLAGS --out=$NDK_DIR/$NDK_STACK_PACKAGE &&
package_it "ndk-stack binary" ndk-stack-$HOST_TAG $NDK_STACK_PACKAGE


# If no sysroot is specified, build one explicitely
if [ -z "$OPTION_SYSROOT" ]; then
    SYSROOT=$BUILD_DIR/platforms/android-9/arch-$ARCH
    mkdir -p $SYSROOT
    dump "Creating sysroot: $SYSROOT"
    $PROGDIR/build-platforms.sh --arch=$ARCH --no-samples --no-symlinks --dst-dir=$BUILD_DIR
    OPTION_SYSROOT=$SYSROOT
fi

# Build the toolchain from sources
# $1: toolchain name (e.g. arm-linux-androideabi-4.4.3)
# $2: extra flags, if needed
build_toolchain ()
{
    dump "Building $1 toolchain... (this can be long)"
    run $PROGDIR/build-gcc.sh $FLAGS $2 --sysroot=$OPTION_SYSROOT --mpfr-version=$MPFR_VERSION --binutils-version=$BINUTILS_VERSION --gdb-version=$GDB_VERSION --build-out=$BUILD_DIR/toolchain-$1 $SRC_DIR $NDK_DIR $1
    fail_panic "Could not build $1 toolchain!"
    package_it "$1 toolchain" "$1-$HOST_TAG" "toolchains/$1/prebuilt/$HOST_TAG"
}

build_gdbserver ()
{
    if [ "$MINGW" = yes ] ; then
        dump "Skipping gdbserver build (--mingw option being used)."
        return
    fi
    dump "Build $1 gdbserver..."
    $PROGDIR/build-gdbserver.sh $FLAGS --sysroot=$OPTION_SYSROOT --build-out=$BUILD_DIR/gdbserver-$1 --gdb-version=$GDB_VERSION $SRC_DIR $NDK_DIR $1
    fail_panic "Could not build $1 gdbserver!"
    package_it "$1 gdbserver" "$1-gdbserver" "toolchains/$1/prebuilt/gdbserver"
}

get_toolchain_sources

case "$ARCH" in
arm )
    build_toolchain $DEFAULT_ARCH_TOOLCHAIN_NAME_arm
    if [ -z "$HOST_ONLY" ]; then
        build_gdbserver $DEFAULT_ARCH_TOOLCHAIN_NAME_arm
    fi
    ;;
x86 )
    build_toolchain $DEFAULT_ARCH_TOOLCHAIN_NAME_x86
    if [ -z "$HOST_ONLY" ]; then
        build_gdbserver $DEFAULT_ARCH_TOOLCHAIN_NAME_x86
    fi
    ;;
esac

if [ -z "$HOST_ONLY" ]; then
    # We need to package the libsupc++ binaries on Linux since the GCC build
    # scripts cannot build them with --mingw option.
    if [ "$HOST_OS" = "linux" ] ; then
        case "$ARCH" in
        arm )
            LIBSUPC_DIR="toolchains/$DEFAULT_ARCH_TOOLCHAIN_NAME_arm/prebuilt/$HOST_TAG/$DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm/lib"
            package_it "GNU libsupc++ armeabi libs" "gnu-libsupc++-armeabi" "$LIBSUPC_DIR/libsupc++.a $LIBSUPC_DIR/thumb/libsupc++.a"
            package_it "GNU libsupc++ armeabi-v7a libs" "gnu-libsupc++-armeabi-v7a" "$LIBSUPC_DIR/armv7-a/libsupc++.a $LIBSUPC_DIR/armv7-a/thumb/libsupc++.a"
            ;;
        x86 )
            LIBSUPC_DIR="toolchains/$DEFAULT_ARCH_TOOLCHAIN_NAME_x86/prebuilt/$HOST_TAG/$DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86/lib"
            package_it "GNU libsupc++ x86 libs" "gnu-libsupc++-x86" "$LIBSUPC_DIR/libsupc++.a"
            ;;
        esac
    fi

    if [ "$MINGW" != "yes" ] ; then
        package_it "GNU libstdc++ headers" "gnu-libstdc++-headers" "sources/cxx-stl/gnu-libstdc++/include"

        case "$ARCH" in
        arm )
            package_it "GNU libstdc++ armeabi libs" "gnu-libstdc++-libs-armeabi" "sources/cxx-stl/gnu-libstdc++/libs/armeabi"
            package_it "GNU libstdc++ armeabi-v7a libs" "gnu-libstdc++-libs-armeabi-v7a" "sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a"
            ;;
        x86 )
            package_it "GNU libstdc++ x86 libs" "gnu-libstdc++-libs-x86" "sources/cxx-stl/gnu-libstdc++/libs/x86"
            ;;
        esac
    fi

    # Rebuild GAbi++ prebuilt libraries
    if [ "$MINGW" != "yes" ]; then
        dump "Building GAbi++ binaries"
        BUILD_GABIXX_FLAGS="--ndk-dir=\"$NDK_DIR\" --package-dir=\"$PACKAGE_DIR\""
        if [ $VERBOSE = yes ] ; then
            BUILD_GABIXX_FLAGS="$BUILD_GABIXX_FLAGS --verbose"
        fi
        $ANDROID_NDK_ROOT/build/tools/build-platforms.sh --no-symlinks --no-samples --arch=$ARCH --dst-dir="$NDK_DIR"
        case "$ARCH" in
        arm )
			BUILD_GABIXX_FLAGS=$BUILD_GABIXX_FLAGS" --abis=armeabi,armeabi-v7a"
            ;;
        x86 )
			BUILD_GABIXX_FLAGS=$BUILD_GABIXX_FLAGS" --abis=x86"
            ;;
        esac
        $ANDROID_NDK_ROOT/build/tools/build-gabi++.sh $BUILD_GABIXX_FLAGS
    else
        dump "Skipping GAbi++ binaries build (--mingw option being used)"
    fi

    # Rebuild STLport prebuilt libraries
    if [ "$MINGW" != "yes" ] ; then
        dump "Building STLport binaries"
        BUILD_STLPORT_FLAGS="--ndk-dir=\"$NDK_DIR\" --package-dir=\"$PACKAGE_DIR\""
        if [ $VERBOSE = yes ] ; then
            BUILD_STLPORT_FLAGS="$BUILD_STLPORT_FLAGS --verbose"
        fi
        $ANDROID_NDK_ROOT/build/tools/build-platforms.sh --no-symlinks --no-samples --arch=$ARCH --dst-dir="$NDK_DIR"
        case "$ARCH" in
        arm )
            BUILD_STLPORT_FLAGS=$BUILD_STLPORT_FLAGS" --abis=armeabi,armeabi-v7a"
            ;;
        x86 )
            BUILD_STLPORT_FLAGS=$BUILD_STLPORT_FLAGS" --abis=x86"
            ;;
        esac
        $ANDROID_NDK_ROOT/build/tools/build-stlport.sh $BUILD_STLPORT_FLAGS
    else
        dump "Skipping STLport binaries build (--mingw option being used)"
    fi
fi # !HOST_ONLY

if [ -n "$PACKAGE_DIR" ] ; then
    dump "Done! See $PACKAGE_DIR"
else
    dump "Done! See $NDK_DIR/toolchains"
fi
