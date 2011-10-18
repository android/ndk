#!/bin/sh
#
# Copyright (C) 2009-2010 The Android Open Source Project
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
# This script is used to package complete Android NDK release packages.
#
# You will need prebuilt toolchain binary tarballs or a previous
# NDK release package to do that.
#
# See make-release.sh if you want to make a new release completely from
# scratch.
#

. `dirname $0`/prebuilt-common.sh

NDK_ROOT_DIR="$ANDROID_NDK_ROOT"

# the list of platforms / API levels we want to package in
# this release. This can be overriden with the --platforms
# option, see below.
#
PLATFORMS="$API_LEVELS"

# the default release name (use today's date)
RELEASE=`date +%Y%m%d`
register_var_option "--release=<name>" RELEASE "Specify release name"

# the directory containing all prebuilts
PREBUILT_DIR=
register_var_option "--prebuilt-dir=<path>" PREBUILT_DIR "Specify prebuilt directory"

# a prebuilt NDK archive (.zip file). empty means don't use any
PREBUILT_NDK=
register_var_option "--prebuilt-ndk=<file>" PREBUILT_NDK "Specify prebuilt ndk package"

# the list of supported host development systems
SYSTEMS=$DEFAULT_SYSTEMS
register_var_option "--systems=<list>" SYSTEMS "Specify host systems"

# ARCH to build for
ARCHS=$DEFAULT_ARCHS
register_var_option "--arch=<arch>" ARCHS "Specify target architecture(s)"

# set to 'yes' if we should use 'git ls-files' to list the files to
# be copied into the archive.
NO_GIT=no
register_var_option "--no-git" NO_GIT "Don't use git to list input files, take all of them."

# set of toolchain prebuilts we need to package
TOOLCHAINS="$DEFAULT_ARCH_TOOLCHAIN_arm"
OPTION_TOOLCHAINS=$TOOLCHAINS
register_var_option "--toolchains=<list>" OPTION_TOOLCHAINS "Specify list of toolchains."

# set of platforms to package (all by default)
register_var_option "--platforms=<list>" PLATFORMS "Specify API levels"

# the package prefix
PREFIX=android-ndk
register_var_option "--prefix=<name>" PREFIX "Specify package prefix"

# default location for generated packages
OUT_DIR=/tmp/ndk-$USER/release
OPTION_OUT_DIR=
register_var_option "--out-dir=<path>" OPTION_OUT_DIR "Specify output package directory" "$OUT_DIR"

# Find the location of the platforms root directory
DEVELOPMENT_ROOT=`dirname $ANDROID_NDK_ROOT`/development/ndk
register_var_option "--development-root=<path>" DEVELOPMENT_ROOT "Specify platforms/samples directory"

PROGRAM_PARAMETERS=
PROGRAM_DESCRIPTION=\
"Package a new set of release packages for the Android NDK.

You will need to have generated one or more prebuilt binary tarballs
with the build/tools/rebuild-all-prebuilts.sh script. These files should
be named like <toolname>-<system>.tar.bz2, where <toolname> is an arbitrary
tool name, and <system> is one of: $SYSTEMS

Use the --prebuilt-dir=<path> option to build release packages from the
binary tarballs stored in <path>.

Alternatively, you can use --prebuilt-ndk=<file> where <file> is the path
to a previous NDK release package. It will be used to extract the toolchain
binaries and copy them to your new release. Only use this for experimental
package releases.

The generated release packages will be stored in a temporary directory that
will be printed at the end of the generation process.
"

extract_parameters "$@"

# Ensure that SYSTEMS is space-separated
SYSTEMS=$(commas_to_spaces $SYSTEMS)

# Do we need to support x86?
ARCHS=$(commas_to_spaces $ARCHS)
echo "$ARCHS" | tr ' ' '\n' | grep -q x86
if [ $? = 0 ] ; then
    TRY_X86=yes
else
    TRY_X86=no
fi

# Compute ABIS from ARCHS
ABIS=
for ARCH in $ARCHS; do
    DEFAULT_ABIS=$(get_default_abis_for_arch $ARCH)
    if [ -z "$ABIS" ]; then
        ABIS=$DEFAULT_ABIS
    else
        ABIS=$ABIS" $DEFAULT_ABIS"
    fi
done

# If --arch is used to list x86 as a target architecture, Add x86-4.4.3 to
# the list of default toolchains to package. That is, unless you also
# explicitely use --toolchains=<list>
#
# Ensure that TOOLCHAINS is space-separated after this.
#
if [ "$OPTION_TOOLCHAINS" != "$TOOLCHAINS" ]; then
    TOOLCHAINS=$(commas_to_spaces $OPTION_TOOLCHAINS)
else
    if [ "$TRY_X86" = "yes" ]; then
        TOOLCHAINS="$TOOLCHAINS $DEFAULT_ARCH_TOOLCHAIN_x86"
    fi
    TOOLCHAINS=$(commas_to_spaces $TOOLCHAINS)
fi

# Check the prebuilt path
#
if [ -n "$PREBUILT_NDK" -a -n "$PREBUILT_DIR" ] ; then
    echo "ERROR: You cannot use both --prebuilt-ndk and --prebuilt-dir at the same time."
    exit 1
fi

if [ -z "$PREBUILT_DIR" -a -z "$PREBUILT_NDK" ] ; then
    echo "ERROR: You must use one of --prebuilt-dir or --prebuilt-ndk. See --help for details."
    exit 1
fi

# Check the option directory.
if [ -n "$OPTION_OUT_DIR" ] ; then
    OUT_DIR="$OPTION_OUT_DIR"
    mkdir -p $OUT_DIR
    if [ $? != 0 ] ; then
        echo "ERROR: Could not create output directory: $OUT_DIR"
        exit 1
    fi
else
    rm -rf $OUT_DIR && mkdir -p $OUT_DIR
fi

# Handle the prebuilt binaries now
#
if [ -n "$PREBUILT_DIR" ] ; then
    if [ ! -d "$PREBUILT_DIR" ] ; then
        echo "ERROR: the --prebuilt-dir argument is not a directory: $PREBUILT_DIR"
        exit 1
    fi
    if [ -z "$SYSTEMS" ] ; then
        echo "ERROR: Your systems list is empty, use --systems=LIST to specify a different one."
        exit 1
    fi
    # Check the toolchain prebuilts
    #
    for TC in $TOOLCHAINS; do
        for SYS in $SYSTEMS; do
            if [ ! -f "$PREBUILT_DIR/$TC-$SYS.tar.bz2" ] ; then
                echo "ERROR: Missing prebuilt file $TC-$SYS.tar.bz2 in: $PREBUILT_DIR"
                exit 1
            fi
        done
        if [ ! -f "$PREBUILT_DIR/$TC-gdbserver.tar.bz2" ] ; then
            echo "ERROR: Missing prebuilt file $TC-gdbserver.tar.bz2 in: $PREBUILT_DIR"
            exit 1
        fi
    done
else
    if [ ! -f "$PREBUILT_NDK" ] ; then
        echo "ERROR: the --prebuilt-ndk argument is not a file: $PREBUILT_NDK"
        exit 1
    fi
    # Check that the name ends with the proper host tag
    HOST_NDK_SUFFIX="$HOST_TAG.zip"
    echo "$PREBUILT_NDK" | grep -q "$HOST_NDK_SUFFIX"
    fail_panic "The name of the prebuilt NDK must end in $HOST_NDK_SUFFIX"
    SYSTEMS=$HOST_TAG
fi

echo "Architectures: $ARCHS"
echo "CPU ABIs: $ABIS"
echo "Toolchains: $TOOLCHAINS"
echo "Host systems: $SYSTEMS"


# The list of git files to copy into the archives
if [ "$NO_GIT" != "yes" ] ; then
    echo "Collecting sources from git (use --no-git to copy all files instead)."
    GIT_FILES=`cd $NDK_ROOT_DIR && git ls-files`
else
    echo "Collecting all sources files under tree."
    # Cleanup everything that is likely to not be part of the final NDK
    # i.e. generated files...
    rm -rf $NDK_ROOT_DIR/samples/*/obj
    rm -rf $NDK_ROOT_DIR/samples/*/libs
    # Get all files under the NDK root
    GIT_FILES=`cd $NDK_ROOT_DIR && find .`
    GIT_FILES=`echo $GIT_FILES | sed -e "s!\./!!g"`
fi

# temporary directory used for packaging
TMPDIR=$NDK_TMPDIR

RELEASE_PREFIX=$PREFIX-$RELEASE

# ensure that the generated files are ug+rx
umask 0022

# Unpack a prebuilt into the destination directory ($DSTDIR)
# $1: prebuilt name, relative to $PREBUILT_DIR
# $2: optional, destination directory
unpack_prebuilt ()
{
    local PREBUILT=$1
    local DDIR="${2:-$DSTDIR}"
    echo "Unpacking $PREBUILT"
    if [ -f "$PREBUILT_DIR/$PREBUILT" ] ; then
        unpack_archive "$PREBUILT_DIR/$PREBUILT" "$DDIR"
        fail_panic "Could not unpack prebuilt $PREBUILT. Aborting."
    else
        echo "WARNING: Could not find $PREBUILT in $PREBUILT_DIR"
    fi
}

# Copy a prebuilt directory from the previous
# $1: Source directory relative to
copy_prebuilt ()
{
    local SUBDIR="$1"
    if [ -d "$1" ] ; then
        echo "Copying: $SUBDIR"
        copy_directory "$SUBDIR" "$DSTDIR/$2"
    else
        echo "Ignored: $SUBDIR"
    fi
}


rm -rf $TMPDIR && mkdir -p $TMPDIR

# Unpack the previous NDK package if any
if [ -n "$PREBUILT_NDK" ] ; then
    echo "Unpacking prebuilt toolchains from $PREBUILT_NDK"
    UNZIP_DIR=$TMPDIR/prev-ndk
    rm -rf $UNZIP_DIR && mkdir -p $UNZIP_DIR
    fail_panic "Could not create temporary directory: $UNZIP_DIR"
    unpack_archive "$PREBUILT_NDK" "$UNZIP_DIR"
    fail_panic "Could not unzip NDK package $PREBUILT_NDK"
fi

# first create the reference ndk directory from the git reference
echo "Creating reference from source files"
REFERENCE=$TMPDIR/reference && rm -rf $REFERENCE/* &&
copy_file_list "$NDK_ROOT_DIR" "$REFERENCE" "$GIT_FILES" &&
rm -f $REFERENCE/Android.mk
fail_panic "Could not create reference. Aborting."

# Copy platform and sample files
if [ "$PREBUILT_DIR" ]; then
    echo "Unpacking platform files" &&
    unpack_archive "$PREBUILT_DIR/platforms.tar.bz2" "$REFERENCE" &&
    echo "Unpacking samples files" &&
    unpack_archive "$PREBUILT_DIR/samples.tar.bz2" "$REFERENCE"
    fail_panic "Could not unpack platform and sample files"
elif [ "$PREBUILT_NDK" ]; then
    echo "ERROR: NOT IMPLEMENTED!"
    exit 1
else
    # copy platform and sample files
    echo "Copying platform and sample files"
    FLAGS="--src-dir=$DEVELOPMENT_ROOT --dst-dir=$REFERENCE"
    if [ "$VERBOSE2" = "yes" ] ; then
        FLAGS="$FLAGS --verbose"
    fi

    FLAGS="$FLAGS --platform=$(spaces_to_commas $PLATFORMS)"
    FLAGS="$FLAGS --arch=$(spaces_to_commas $ARCHS)"
    $NDK_ROOT_DIR/build/tools/gen-platforms.sh $FLAGS
    fail_panic "Could not copy platform files. Aborting."
fi

# Remove the source for host tools to make the final package smaller
rm -rf $REFERENCE/sources/host-tools

# Remove leftovers, just in case...
rm -rf $REFERENCE/samples/*/{obj,libs,build.xml,local.properties,Android.mk} &&
rm -rf $REFERENCE/tests/build/*/{obj,libs} &&
rm -rf $REFERENCE/tests/device/*/{obj,libs}

# copy sources files
if [ -d $DEVELOPMENT_ROOT/sources ] ; then
    echo "Copying NDK sources files"
    copy_file_list "$DEVELOPMENT_ROOT" "$REFERENCE" "sources"
    fail_panic "Could not copy sources. Aborting."
fi

# Unpack prebuilt C++ runtimes headers and libraries
if [ -z "$PREBUILT_NDK" ]; then
    # Unpack gdbserver
    for TC in $TOOLCHAINS; do
        unpack_prebuilt $TC-gdbserver.tar.bz2 "$REFERENCE"
    done
    # Unpack C++ runtimes
    unpack_prebuilt gnu-libstdc++-headers.tar.bz2 "$REFERENCE"
    for ABI in $ABIS; do
        unpack_prebuilt gabixx-libs-$ABI.tar.bz2 "$REFERENCE"
        unpack_prebuilt stlport-libs-$ABI.tar.bz2 "$REFERENCE"
        unpack_prebuilt gnu-libstdc++-libs-$ABI.tar.bz2 "$REFERENCE"
    done
fi

# create a release file named 'RELEASE.TXT' containing the release
# name. This is used by the build script to detect whether you're
# invoking the NDK from a release package or from the development
# tree.
#
echo "$RELEASE" > $REFERENCE/RELEASE.TXT

# Remove un-needed files
rm -f $REFERENCE/CleanSpec.mk

# now, for each system, create a package
#
for SYSTEM in $SYSTEMS; do
    echo "Preparing package for system $SYSTEM."
    BIN_RELEASE=$RELEASE_PREFIX-$SYSTEM
    DSTDIR=$TMPDIR/$RELEASE_PREFIX
    rm -rf $DSTDIR &&
    copy_file_list "$REFERENCE" "$DSTDIR" "*"
    fail_panic "Could not copy reference. Aborting."

    if [ "$PREBUILT_NDK" ]; then
        cd $UNZIP_DIR/android-ndk-* && cp -rP toolchains/* $DSTDIR/toolchains/
        fail_panic "Could not copy toolchain files from $PREBUILT_NDK"

        if [ -d "$DSTDIR/$GABIXX_SUBDIR" ]; then
            GABIXX_ABIS=$PREBUILT_ABIS
            for GABIXX_ABI in $GABIXX_ABIS; do
                copy_prebuilt "$GABIXX_SUBDIR/libs/$GABIXX_ABI" "$GABIXX_SUBDIR/libs"
            done
        else
            echo "WARNING: Could not find GAbi++ source tree!"
        fi

        if [ -d "$DSTDIR/$STLPORT_SUBDIR" ] ; then
            STLPORT_ABIS=$PREBUILT_ABIS
            for STL_ABI in $STLPORT_ABIS; do
                copy_prebuilt "$STLPORT_SUBDIR/libs/$STL_ABI" "$STLPORT_SUBDIR/libs"
            done
        else
            echo "WARNING: Could not find STLport source tree!"
        fi

        copy_prebuilt "$GNUSTL_SUBDIR/include" "$GNUSTL_SUBDIR"
        for STL_ABI in $PREBUILT_ABIS; do
            copy_prebuilt "$GNUSTL_SUBDIR/libs/$STL_ABI" "$GNUSTL_SUBDIR/libs"
        done
    else
        # Unpack gdbserver
        for TC in $TOOLCHAINS; do
            unpack_prebuilt $TC-$SYSTEM.tar.bz2
            echo "Removing sysroot for $TC"
            rm -rf $DSTDIR/toolchains/$TC/prebuilt/$SYSTEM/sysroot
        done

        # Unpack prebuilt ndk-stack and other host tools
        unpack_prebuilt ndk-stack-$SYSTEM.tar.bz2
        unpack_prebuilt ndk-make-$SYSTEM.tar.bz2
        unpack_prebuilt ndk-sed-$SYSTEM.tar.bz2
        unpack_prebuilt ndk-awk-$SYSTEM.tar.bz2

        if [ "$SYSTEM" = "windows" ]; then
            unpack_prebuilt toolbox-$SYSTEM.tar.bz2
        fi
    fi

    # Create an archive for the final package. Extension depends on the
    # host system.
    case "$SYSTEM" in
        windows)
            ARCHIVE="$BIN_RELEASE.zip"
            ;;
        *)
            ARCHIVE="$BIN_RELEASE.tar.bz2"
            ;;
    esac
    echo "Creating $ARCHIVE"
    pack_archive "$OUT_DIR/$ARCHIVE" "$TMPDIR" "$RELEASE_PREFIX"
    fail_panic "Could not create archive: $OUT_DIR/$ARCHIVE"
#    chmod a+r $TMPDIR/$ARCHIVE
done

echo "Cleaning up."
rm -rf $TMPDIR/reference
rm -rf $TMPDIR/prev-ndk

echo "Done, please see packages in $OUT_DIR:"
ls -l $OUT_DIR
