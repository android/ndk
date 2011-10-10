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

# Create a standalone toolchain package for Android.

. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"Generate a customized Android toolchain installation that includes
a working sysroot. The result is something that can more easily be
used as a standalone cross-compiler, e.g. to run configure and
make scripts."

force_32bit_binaries

# For now, this is the only toolchain that works reliably.
TOOLCHAIN_NAME=
register_var_option "--toolchain=<name>" TOOLCHAIN_NAME "Specify toolchain name"

ARCH=
register_option "--arch=<name>" do_arch "Specify target architecture" "arm"
do_arch () { ARCH=$1; }

NDK_DIR=`dirname $0`
NDK_DIR=`dirname $NDK_DIR`
NDK_DIR=`dirname $NDK_DIR`
register_var_option "--ndk-dir=<path>" NDK_DIR "Take source files from NDK at <path>"

SYSTEM=$HOST_TAG
register_var_option "--system=<name>" SYSTEM "Specify host system"

PACKAGE_DIR=/tmp/ndk-$USER
register_var_option "--package-dir=<path>" PACKAGE_DIR "Place package file in <path>"

INSTALL_DIR=
register_var_option "--install-dir=<path>" INSTALL_DIR "Don't create package, install files to <path> instead."

PLATFORM=
register_option "--platform=<name>" do_platform "Specify target Android platform/API level." "android-3"
do_platform () { PLATFORM=$1; }

extract_parameters "$@"

# Check NDK_DIR
if [ ! -d "$NDK_DIR/build/core" ] ; then
    echo "Invalid source NDK directory: $NDK_DIR"
    echo "Please use --ndk-dir=<path> to specify the path of an installed NDK."
    exit 1
fi

# Check ARCH
if [ -z "$ARCH" ]; then
    case $TOOLCHAIN_NAME in
        arm-*)
            ARCH=arm
            ;;
        x86-*)
            ARCH=x86
            ;;
        *)
            ARCH=arm
            ;;
    esac
    log "Auto-config: --arch=$ARCH"
fi

# Check toolchain name
if [ -z "$TOOLCHAIN_NAME" ]; then
    TOOLCHAIN_NAME=$(get_default_toolchain_name_for_arch $ARCH)
    echo "Auto-config: --toolchain=$TOOLCHAIN_NAME"
fi

# Check PLATFORM
if [ -z "$PLATFORM" ]; then
    case $ARCH in
        arm) PLATFORM=android-3
            ;;
        x86)
            PLATFORM=android-9
            ;;
    esac
    log "Auto-config: --platform=$PLATFORM"
fi

if [ ! -d "$NDK_DIR/platforms/$PLATFORM" ] ; then
    echo "Invalid platform name: $PLATFORM"
    echo "Please use --platform=<name> with one of:" `(cd "$NDK_DIR/platforms" && ls)`
    exit 1
fi

# Check toolchain name
TOOLCHAIN_PATH="$NDK_DIR/toolchains/$TOOLCHAIN_NAME"
if [ ! -d "$TOOLCHAIN_PATH" ] ; then
    echo "Invalid toolchain name: $TOOLCHAIN_NAME"
    echo "Please use --toolchain=<name> with the name of a toolchain supported by the source NDK."
    echo "Try one of: " `(cd "$NDK_DIR/toolchains" && ls)`
    exit 1
fi

# Extract architecture from platform name
case "$TOOLCHAIN_NAME" in
    arm-*)
        ;;
    x86-*)
        ;;
    *)
        echo "Unsupported toolchain name: $TOOLCHAIN_NAME"
        echo "Name must start with arm- or x86- !"
        exit 1
        ;;
esac

# Check that there are any platform files for it!
(cd $NDK_DIR/platforms && ls -d */arch-${ARCH} >/dev/null 2>&1 )
if [ $? != 0 ] ; then
    echo "Platform $PLATFORM doesn't have any files for this architecture: $ARCH"
    echo "Either use --platform=<name> or --toolchain=<name> to select a different"
    echo "platform or arch-dependent toolchain name (respectively)!"
    exit 1
fi

# Compute source sysroot
SRC_SYSROOT="$NDK_DIR/platforms/$PLATFORM/arch-$ARCH"
if [ ! -d "$SRC_SYSROOT" ] ; then
    echo "No platform files ($PLATFORM) for this architecture: $ARCH"
    exit 1
fi

# Check that we have any prebuilts here
if [ ! -d "$TOOLCHAIN_PATH/prebuilt" ] ; then
    echo "Toolchain is missing prebuilt files: $TOOLCHAIN_NAME"
    echo "You must point to a valid NDK release package!"
    exit 1
fi

if [ ! -d "$TOOLCHAIN_PATH/prebuilt/$SYSTEM" ] ; then
    echo "Host system '$SYSTEM' is not supported by the source NDK!"
    echo "Try --system=<name> with one of: " `(cd $TOOLCHAIN_PATH/prebuilt && ls) | grep -v gdbserver`
    exit 1
fi

TOOLCHAIN_PATH="$TOOLCHAIN_PATH/prebuilt/$SYSTEM"

# Create temporary directory
TMPDIR=$NDK_TMPDIR/standalone/$TOOLCHAIN_NAME

dump "Copying prebuilt binaries..."
# Now copy the toolchain prebuilt binaries
run copy_directory "$TOOLCHAIN_PATH" "$TMPDIR"

dump "Copying sysroot headers and libraries..."
# Copy the sysroot under $TMPDIR/sysroot. The toolchain was built to
# expect the sysroot files to be placed there!
run copy_directory_nolinks "$SRC_SYSROOT" "$TMPDIR/sysroot"

dump "Copying libstdc++ headers and libraries..."
`dirname $0`/copy-libstdcxx.sh --reverse "$TMPDIR" "$NDK_DIR" --toolchain=$TOOLCHAIN_NAME
fail_panic "Could not copy libstdc++!"

# Install or Package
if [ -n "$INSTALL_DIR" ] ; then
    dump "Copying files to: $INSTALL_DIR"
    run copy_directory "$TMPDIR" "$INSTALL_DIR"
else
    PACKAGE_FILE="$PACKAGE_DIR/$TOOLCHAIN_NAME.tar.bz2"
    dump "Creating package file: $PACKAGE_FILE"
    pack_archive "$PACKAGE_FILE" "`dirname $TMPDIR`" "$TOOLCHAIN_NAME"
    fail_panic "Could not create tarball from $TMPDIR"
fi
dump "Cleaning up..."
run rm -rf $TMPDIR

dump "Done."
