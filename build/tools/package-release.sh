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
PLATFORMS="3 4 5 8 9"

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
SYSTEMS="linux-x86 darwin-x86 windows"
register_var_option "--systems=<list>" SYSTEMS "Specify host systems"

# set to 'yes' if we should use 'git ls-files' to list the files to
# be copied into the archive.
NO_GIT=no
register_var_option "--no-git" NO_GIT "Don't use git to list input files, take all of them."

# set of toolchain prebuilts we need to package
TOOLCHAINS="arm-eabi-4.4.0 arm-linux-androideabi-4.4.3"
register_var_option "--toolchains=<list>" TOOLCHAINS "Specify list of toolchains."

# set of platforms to package (all by default)
register_var_option "--platforms=<list>" PLATFORMS "Specify API levels"

# the package prefix
PREFIX=android-ndk
register_var_option "--prefix=<name>" PREFIX "Specify package prefix"

# default location for generated packages
OUT_DIR=/tmp/ndk-release
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

extract_parameters $@

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
    if [ $? != 0 ] ; then
        echo "ERROR: the name of the prebuilt NDK must end in $HOST_NDK_SUFFIX"
        exit 1
    fi
    SYSTEMS=$HOST_TAG
fi

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
TMPDIR=/tmp/ndk-release

RELEASE_PREFIX=$PREFIX-$RELEASE

# ensure that the generated files are ug+rx
umask 0022

rm -rf $TMPDIR && mkdir -p $TMPDIR

# first create the reference ndk directory from the git reference
echo "Creating reference from source files"
REFERENCE=$TMPDIR/reference &&
mkdir -p $REFERENCE &&
(cd $NDK_ROOT_DIR && tar cf - $GIT_FILES) | (cd $REFERENCE && tar xf -) &&
rm -f $REFERENCE/Android.mk
if [ $? != 0 ] ; then
    echo "Could not create reference. Aborting."
    exit 2
fi

# copy platform and sample files
echo "Copying platform and sample files"
FLAGS="--src-dir=$DEVELOPMENT_ROOT --dst-dir=$REFERENCE"
if [ "$VERBOSE2" = "yes" ] ; then
  FLAGS="$FLAGS --verbose"
fi
PLATFORM_FLAGS=
if [ -n "$PLATFORMS" ] ; then
    $NDK_ROOT_DIR/build/tools/build-platforms.sh $FLAGS --platform="$PLATFORMS"
else
    $NDK_ROOT_DIR/build/tools/build-platforms.sh $FLAGS $PLATFORM_FLAGS
fi
if [ $? != 0 ] ; then
    echo "Could not copy platform files. Aborting."
    exit 2
fi

# copy sources files
if [ -d $DEVELOPMENT_ROOT/sources ] ; then
    echo "Copying NDK sources files"
    (cd $DEVELOPMENT_ROOT && tar cf - sources) | (cd $REFERENCE && tar xf -)
    if [ $? != 0 ] ; then
        echo "Could not copy sources. Aborting."
        exit 2
    fi
fi

# create a release file named 'RELEASE.TXT' containing the release
# name. This is used by the build script to detect whether you're
# invoking the NDK from a release package or from the development
# tree.
#
echo "$RELEASE" > $REFERENCE/RELEASE.TXT

# Remove un-needed files
rm -f $REFERENCE/CleanSpec.mk

# Unpack a prebuilt into the destination directory ($DSTDIR)
# $1: prebuilt name, relative to $PREBUILT_DIR
unpack_prebuilt ()
{
    local PREBUILT=$1
    echo "Unpacking $PREBUILT"
    if [ -f "$PREBUILT_DIR/$PREBUILT" ] ; then
        (cd $DSTDIR && run tar xjf "$PREBUILT_DIR/$PREBUILT") 2>/dev/null 1>&2
        if [ $? != 0 ] ; then
            echo "Could not unpack prebuilt $PREBUILT. Aborting."
            exit 1
        fi
    else
        echo "WARNING: Could not find $PREBUILT in $PREBUILT_DIR"
    fi
}

# now, for each system, create a package
#
for SYSTEM in $SYSTEMS; do
    echo "Preparing package for system $SYSTEM."
    BIN_RELEASE=$RELEASE_PREFIX-$SYSTEM
    DSTDIR=$TMPDIR/$RELEASE_PREFIX
    rm -rf $DSTDIR && mkdir -p $DSTDIR &&
    cp -rp $REFERENCE/* $DSTDIR
    if [ $? != 0 ] ; then
        echo "Could not copy reference. Aborting."
        exit 2
    fi

    if [ -n "$PREBUILT_NDK" ] ; then
        echo "Unpacking prebuilt toolchains from $PREBUILT_NDK"
        UNZIP_DIR=$TMPDIR/prev-ndk
        rm -rf $UNZIP_DIR && mkdir -p $UNZIP_DIR
        if [ $? != 0 ] ; then
            echo "Could not create temporary directory: $UNZIP_DIR"
            exit 1
        fi
        cd $UNZIP_DIR && unzip -q $PREBUILT_NDK 1>/dev/null 2>&1
        if [ $? != 0 ] ; then
            echo "ERROR: Could not unzip NDK package $PREBUILT_NDK"
            exit 1
        fi
        cd $UNZIP_DIR/android-ndk-* && cp -rP toolchains/* $DSTDIR/toolchains/

        if [ -d "$DSTDIR/$STLPORT_SUBDIR" ] ; then
            STLPORT_ABIS="armeabi armeabi-v7a x86"
            cd $UNZIP_DIR/android-ndk-*
            for STL_ABI in $STLPORT_ABIS; do
                if [ -d "$STLPORT_SUBDIR/$STL_ABI" ] ; then
                    echo "Copying: $STLPORT_SUBDIR/$STL_ABI"
                    run cp -rp $STLPORT_SUBDIR/$STL_ABI $DSTDIR/$STLPORT_SUBDIR
                else
                    echo "Ignored: $STLPORT_SUBDIR/$STL_ABI"
                fi
            done
        else
            echo "WARNING: Could not find STLport source tree!"
        fi
    else
        for TC in $TOOLCHAINS; do
            unpack_prebuilt $TC-$SYSTEM.tar.bz2
            echo "Removing sysroot for $TC"
            rm -rf $DSTDIR/toolchains/$TC/prebuilt/$SYSTEM/sysroot
            unpack_prebuilt $TC-gdbserver.tar.bz2
        done
        unpack_prebuilt stlport-libs-armeabi.tar.bz2
        unpack_prebuilt stlport-libs-armeabi-v7a.tar.bz2
        echo "$TOOLCHAINS" | tr ' ' '\n' | grep -q x86
        if [ $? = 0 ] ; then
            unpack_prebuilt stlport-prebuilt-x86.tar.bz2
        fi

    fi

    # Create an archive for the final package. Extension depends on the
    # host system.
    case "$SYSTEM" in
        windows)
            ARCHIVE="$BIN_RELEASE.zip"
            PACKER="zip -9qr"
            ;;
        *)
            ARCHIVE="$BIN_RELEASE.tar.bz2"
            PACKER="tar cjf"
            ;;
    esac
    echo "Creating $ARCHIVE"
    (cd $TMPDIR && $PACKER $OUT_DIR/$ARCHIVE $RELEASE_PREFIX && rm -rf $DSTDIR) 2>/dev/null 1>&2
    if [ $? != 0 ] ; then
        echo "Could not create archive. Aborting."
        exit 1
    fi

#    chmod a+r $TMPDIR/$ARCHIVE
done

echo "Cleaning up."
rm -rf $TMPDIR/reference
rm -rf $TMPDIR/prev-ndk

echo "Done, please see packages in $OUT_DIR:"
ls -l $OUT_DIR
