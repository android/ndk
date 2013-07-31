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
# gen-platforms.sh
#
# This tool is used when packaging a new release, or when developing
# the NDK itself. It will populate DST ($NDK/platforms by default)
# with the content of SRC ($NDK/../development/ndk/platforms/ by default).
#
# The idea is that the content of $SRC/android-N/ only contains stuff
# that is relevant to API level N, and not contain anything that is already
# provided by API level N-1, N-2, etc..
#
# More precisely, for each architecture A:
#  $SRC/android-N/include        --> $DST/android-N/arch-A/usr/include
#  $SRC/android-N/arch-A/include --> $DST/android-N/arch-A/usr/include
#  $SRC/android-N/arch-A/lib     --> $DST/android-N/arch-A/usr/lib
#
# Also, we generate on-the-fly shared dynamic libraries from list of symbols:
#
#  $SRC/android-N/arch-A/symbols --> $DST/android-N/arch-A/usr/lib
#
# Repeat after that for N+1, N+2, etc..
#

PROGDIR=$(dirname "$0")
. "$PROGDIR/prebuilt-common.sh"

# Return the list of platform supported from $1/platforms
# as a single space-separated sorted list of levels. (e.g. "3 4 5 8 9 14")
# $1: source directory
extract_platforms_from ()
{
    if [ -d "$1" ] ; then
        (cd "$1/platforms" && ls -d android-*) | sed -e "s!android-!!" | sort -g | tr '\n' ' '
    else
        echo ""
    fi
}

SRCDIR="../development/ndk"
DSTDIR="$ANDROID_NDK_ROOT"

ARCHS="$DEFAULT_ARCHS"
PLATFORMS=`extract_platforms_from "$SRCDIR"`
NDK_DIR=$ANDROID_NDK_ROOT

OPTION_HELP=no
OPTION_PLATFORMS=
OPTION_SRCDIR=
OPTION_DSTDIR=
OPTION_SAMPLES=
OPTION_FAST_COPY=
OPTION_MINIMAL=
OPTION_ARCH=
OPTION_ABI=
OPTION_DEBUG_LIBS=
OPTION_OVERLAY=
PACKAGE_DIR=

VERBOSE=no
VERBOSE2=no

for opt do
  optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
  case "$opt" in
  --help|-h|-\?) OPTION_HELP=yes
  ;;
  --verbose)
    if [ "$VERBOSE" = "yes" ] ; then
        VERBOSE2=yes
    else
        VERBOSE=yes
    fi
    ;;
  --src-dir=*)
    OPTION_SRCDIR="$optarg"
    ;;
  --dst-dir=*)
    OPTION_DSTDIR="$optarg"
    ;;
  --ndk-dir=*)
    NDK_DIR=$optarg
    ;;
  --platform=*)
    OPTION_PLATFORM=$optarg
    ;;
  --arch=*)
    OPTION_ARCH=$optarg
    ;;
  --abi=*)  # We still support this for backwards-compatibility
    OPTION_ABI=$optarg
    ;;
  --samples)
    OPTION_SAMPLES=yes
    ;;
  --fast-copy)
    OPTION_FAST_COPY=yes
    ;;
  --minimal)
    OPTION_MINIMAL=yes
    ;;
  --package-dir=*)
    PACKAGE_DIR=$optarg
    ;;
  --debug-libs)
    OPTION_DEBUG_LIBS=true
    ;;
  --overlay)
    OPTION_OVERLAY=true
    ;;
  *)
    echo "unknown option '$opt', use --help"
    exit 1
  esac
done

if [ $OPTION_HELP = "yes" ] ; then
    echo "Collect files from an Android NDK development tree and assemble"
    echo "the platform files appropriately into a final release structure."
    echo ""
    echo "options:"
    echo ""
    echo "  --help                Print this message"
    echo "  --verbose             Enable verbose messages"
    echo "  --src-dir=<path>      Source directory for development platform files [$SRCDIR]"
    echo "  --dst-dir=<path>      Destination directory [$DSTDIR]"
    echo "  --ndk-dir=<path>      Use toolchains from this NDK directory [$NDK_DIR]"
    echo "  --platform=<list>     List of API levels [$PLATFORMS]"
    echo "  --arch=<list>         List of CPU architectures [$ARCHS]"
    echo "  --minimal             Ignore samples, symlinks and generated shared libs."
    echo "  --fast-copy           Don't create symlinks, copy files instead"
    echo "  --samples             Also generate samples directories."
    echo "  --package-dir=<path>  Package platforms archive in specific path."
    echo "  --debug-libs          Also generate C source file for generated libraries."
    echo ""
    echo "Use the --minimal flag if you want to generate minimal sysroot directories"
    echo "that will be used to generate prebuilt toolchains. Otherwise, the script"
    echo "will require these toolchains to be pre-installed and will use them to"
    echo "generate shared system shared libraries from the symbol list files."
    exit 0
fi

if [ -n "$OPTION_SRCDIR" ] ; then
    SRCDIR="$OPTION_SRCDIR";
    if [ ! -d "$SRCDIR" ] ; then
        echo "ERROR: Source directory $SRCDIR does not exist !"
        exit 1
    fi
    if [ ! -d "$SRCDIR/platforms/android-3" ] ; then
        echo "ERROR: Invalid source directory: $SRCDIR"
        echo "Please make sure it contains platforms/android-3 etc..."
        exit 1
    fi
else
    SRCDIR=`dirname $ANDROID_NDK_ROOT`/development/ndk
    log "Using source directory: $SRCDIR"
fi

if [ -n "$OPTION_PLATFORM" ] ; then
    PLATFORMS=$(commas_to_spaces $OPTION_PLATFORM)
else
    # Build the list from the content of SRCDIR
    PLATFORMS=`extract_platforms_from "$SRCDIR"`
    log "Using platforms: $PLATFORMS"
fi

# Remove the android- prefix of any platform name
PLATFORMS=$(echo $PLATFORMS | tr ' ' '\n' | sed -e 's!^android-!!g' | tr '\n' ' ')

if [ -n "$OPTION_DSTDIR" ] ; then
    DSTDIR="$OPTION_DSTDIR"
else
    log "Using destination directory: $DSTDIR"
fi

# Handle architecture list
#
# We support both --arch and --abi for backwards compatibility reasons
# --arch is the new hotness, --abi is deprecated.
#
if [ -n "$OPTION_ARCH" ]; then
    OPTION_ARCH=$(commas_to_spaces $OPTION_ARCH)
fi

if [ -n "$OPTION_ABI" ] ; then
    echo "WARNING: --abi=<names> is deprecated. Use --arch=<names> instead!"
    OPTION_ABI=$(commas_to_spaces $OPTION_ABI)
    if [ -n "$OPTION_ARCH" -a "$OPTION_ARCH" != "$OPTION_ABI" ]; then
        echo "ERROR: You can't use both --abi and --arch with different values!"
        exit 1
    fi
    OPTION_ARCH=$OPTION_ABI
fi

if [ -n "$OPTION_ARCH" ] ; then
    ARCHS="$OPTION_ARCH"
fi
log "Using architectures: $(commas_to_spaces $ARCHS)"

log "Checking source platforms."
for PLATFORM in $PLATFORMS; do
    DIR="$SRCDIR/platforms/android-$PLATFORM"
    if [ ! -d $DIR ] ; then
        echo "ERROR: Directory missing: $DIR"
        echo "Please check your --platform=<list> option and try again."
        exit 2
    else
        log "  $DIR"
    fi
done

log "Checking source platform architectures."
BAD_ARCHS=
for ARCH in $ARCHS; do
    eval CHECK_$ARCH=no
done
for PLATFORM in $PLATFORMS; do
    for ARCH in $ARCHS; do
        DIR="$SRCDIR/platforms/android-$PLATFORM/arch-$ARCH"
        if [ -d $DIR ] ; then
            log "  $DIR"
            eval CHECK_$ARCH=yes
        fi
    done
done

if [ "$OPTION_MINIMAL" ]; then
    OPTION_SAMPLES=
    OPTION_FAST_COPY=yes
fi

BAD_ARCHS=
for ARCH in $ARCHS; do
    CHECK=`var_value CHECK_$ARCH`
    log "  $ARCH check: $CHECK"
    if [ "$CHECK" = no ] ; then
        if [ -z "$BAD_ARCHS" ] ; then
            BAD_ARCHS=$ARCH
        else
            BAD_ARCHS="$BAD_ARCHS $ARCH"
        fi
    fi
done

if [ -n "$BAD_ARCHS" ] ; then
    echo "ERROR: Source directory doesn't support these ARCHs: $BAD_ARCHS"
    exit 3
fi

# $1: source directory (relative to $SRCDIR)
# $2: destination directory (relative to $DSTDIR)
# $3: description of directory contents (e.g. "sysroot" or "samples")
copy_src_directory ()
{
    local SDIR="$SRCDIR/$1"
    local DDIR="$DSTDIR/$2"
    if [ -d "$SDIR" ] ; then
        log "Copying $3 from \$SRC/$1 to \$DST/$2."
        mkdir -p "$DDIR" && (cd "$SDIR" && 2>/dev/null tar chf - *) | (tar xf - -C "$DDIR")
        if [ $? != 0 ] ; then
            echo "ERROR: Could not copy $3 directory $SDIR into $DDIR !"
            exit 5
        fi
    fi
}

# $1: source dir
# $2: destination dir
# $3: reverse path
#
symlink_src_directory_inner ()
{
    local files file subdir rev
    mkdir -p "$DSTDIR/$2"
    rev=$3
    files=$(cd $DSTDIR/$1 && ls -1p)
    for file in $files; do
        if [ "$file" = "${file%%/}" ]; then
            log2 "Link \$DST/$2/$file --> $rev/$1/$file"
            ln -s $rev/$1/$file $DSTDIR/$2/$file
        else
            file=${file%%/}
            symlink_src_directory_inner "$1/$file" "$2/$file" "$rev/.."
        fi
    done
}
# Create a symlink-copy of directory $1 into $2
# This function is recursive.
#
# $1: source directory (relative to $SRCDIR)
# $2: destination directory (relative to $DSTDIR)
symlink_src_directory ()
{
    symlink_src_directory_inner "$1" "$2" "$(reverse_path $1)"
}

# Remove unwanted symbols
# $1: symbol file (one symbol per line)
# $2+: Input symbol list
# Out: Input symbol file, without any unwanted symbol listed by $1
remove_unwanted_symbols_from ()
{
  local SYMBOL_FILE="$1"
  shift
  if [ -f "$SYMBOL_FILE" ]; then
    echo "$@" | tr ' ' '\n' | grep -v -F -x -f $SYMBOL_FILE | tr '\n' ' '
  else
    echo "$@"
  fi
}

# Remove unwanted symbols from a library's functions list.
# $1: Architecture name
# $2: Library name (e.g. libc.so)
# $3+: Input symbol list
# Out: Input symbol list without any unwanted symbols.
remove_unwanted_function_symbols ()
{
  local ARCH LIBRARY SYMBOL_FILE
  ARCH=$1
  LIBRARY=$2
  shift; shift
  SYMBOL_FILE=$PROGDIR/unwanted-symbols/$ARCH/$LIBRARY.functions.txt
  remove_unwanted_symbols_from $SYMBOL_FILE "$@"
}

# Same as remove_unwanted_functions_symbols, but for variable names.
#
remove_unwanted_variable_symbols ()
{
  local ARCH LIBRARY SYMBOL_FILE
  ARCH=$1
  LIBRARY=$2
  shift; shift
  SYMBOL_FILE=$PROGDIR/unwanted-symbols/$ARCH/$LIBRARY.variables.txt
  remove_unwanted_symbols_from $SYMBOL_FILE "$@"
}

# $1: Architecture
# Out: compiler command
get_default_compiler_for_arch()
{
    local ARCH=$1
    local TOOLCHAIN_PREFIX EXTRA_CFLAGS CC

    if [ "$(arch_in_unknown_archs $ARCH)" = "yes" ]; then
        TOOLCHAIN_PREFIX="$NDK_DIR/$(get_llvm_toolchain_binprefix $DEFAULT_LLVM_VERSION)"
        CC="$TOOLCHAIN_PREFIX/clang"
        EXTRA_CFLAGS="-emit-llvm -target le32-none-ndk"
    else
        TOOLCHAIN_PREFIX="$NDK_DIR/$(get_default_toolchain_binprefix_for_arch $1)"
        TOOLCHAIN_PREFIX=${TOOLCHAIN_PREFIX%-}
        CC="$TOOLCHAIN_PREFIX-gcc"
        EXTRA_CFLAGS=
    fi

    if [ ! -f "$CC" ]; then
        dump "ERROR: $ARCH toolchain not installed: $CC"
        dump "Important: Use the --minimal flag to use this script without generated system shared libraries."
        dump "This is generally useful when you want to generate the host cross-toolchain programs."
        exit 1
    fi
    echo "$CC $EXTRA_CFLAGS"
}

# $1: library name
# $2: functions list
# $3: variables list
# $4: destination file
# $5: compiler command
gen_shared_lib ()
{
    local LIBRARY=$1
    local FUNCS="$2"
    local VARS="$3"
    local DSTFILE="$4"
    local CC="$5"

    # Now generate a small C source file that contains similarly-named stubs
    echo "/* Auto-generated file, do not edit */" > $TMPC
    local func var
    for func in $FUNCS; do
        echo "void $func(void) {}" >> $TMPC
    done
    for var in $VARS; do
        echo "int $var = 0;" >> $TMPC
    done

    # Build it with our cross-compiler. It will complain about conflicting
    # types for built-in functions, so just shut it up.
    COMMAND="$CC -Wl,-shared,-Bsymbolic -Wl,-soname,$LIBRARY -nostdlib -o $TMPO $TMPC"
    echo "## COMMAND: $COMMAND" > $TMPL
    $COMMAND 1>>$TMPL 2>&1
    if [ $? != 0 ] ; then
        dump "ERROR: Can't generate shared library for: $LIBNAME"
        dump "See the content of $TMPC and $TMPL for details."
        cat $TMPL | tail -10
        exit 1
    fi

    # Copy to our destination now
    local libdir=$(dirname "$DSTFILE")
    mkdir -p "$libdir" && cp -f $TMPO "$DSTFILE"
    if [ $? != 0 ] ; then
        dump "ERROR: Can't copy shared library for: $LIBNAME"
        dump "target location is: $DSTFILE"
        exit 1
    fi

    if [ "$OPTION_DEBUG_LIBS" ]; then
      cp $TMPC $DSTFILE.c
      echo "$FUNCS" > $DSTFILE.functions.txt
      echo "$VARS" > $DSTFILE.variables.txt
    fi
}

# $1: Architecture
# $2: symbol source directory (relative to $SRCDIR)
# $3: destination directory for generated libs (relative to $DSTDIR)
# $4: compiler flags (optional)
gen_shared_libraries ()
{
    local ARCH=$1
    local SYMDIR="$SRCDIR/$2"
    local DSTDIR="$DSTDIR/$3"
    local FLAGS="$4"
    local CC funcs vars numfuncs numvars

    # Let's locate the toolchain we're going to use
    CC=$(get_default_compiler_for_arch $ARCH)" $FLAGS"

    # In certain cases, the symbols directory doesn't exist,
    # e.g. on x86 for PLATFORM < 9
    if [ ! -d "$SYMDIR" ]; then
        return
    fi

    # Let's list the libraries we're going to generate
    LIBS=$( (cd $SYMDIR && 2>/dev/null ls *.functions.txt) | sort -u | sed -e 's!\.functions\.txt$!!g')

    for LIB in $LIBS; do
        funcs=$(cat "$SYMDIR/$LIB.functions.txt" 2>/dev/null)
        vars=$(cat "$SYMDIR/$LIB.variables.txt" 2>/dev/null)
        funcs=$(remove_unwanted_function_symbols $ARCH libgcc.a $funcs)
        funcs=$(remove_unwanted_function_symbols $ARCH $LIB $funcs)
        vars=$(remove_unwanted_variable_symbols $ARCH $LIB $vars)
        numfuncs=$(echo $funcs | wc -w)
        numvars=$(echo $vars | wc -w)
        log "Generating $ARCH shared library for $LIB ($numfuncs functions + $numvars variables)"

        gen_shared_lib $LIB "$funcs" "$vars" "$DSTDIR/$LIB" "$CC"
    done
}

# $1: platform number
# $2: architecture name
# $3: common source directory (for crtbrand.c, etc)
# $4: source directory (for *.S files)
# $5: destination directory
# $6: flags for compiler (optional)
gen_crt_objects ()
{
    local API=$1
    local ARCH=$2
    local COMMON_SRC_DIR="$SRCDIR/$3"
    local SRC_DIR="$SRCDIR/$4"
    local DST_DIR="$DSTDIR/$5"
    local FLAGS="$6"
    local SRC_FILE DST_FILE
    local CC

    if [ ! -d "$SRC_DIR" ]; then
        return
    fi

    # Let's locate the toolchain we're going to use
    CC=$(get_default_compiler_for_arch $ARCH)" $FLAGS"

    CRTBRAND_S=$DST_DIR/crtbrand.s
    log "Generating platform $API crtbrand assembly code: $CRTBRAND_S"
    (cd "$COMMON_SRC_DIR" && mkdir -p `dirname $CRTBRAND_S` && $CC -DPLATFORM_SDK_VERSION=$API -fpic -S -o - crtbrand.c | \
        sed -e '/\.note\.ABI-tag/s/progbits/note/' > "$CRTBRAND_S") 1>>$TMPL 2>&1
    if [ $? != 0 ]; then
        dump "ERROR: Could not generate $CRTBRAND_S from $COMMON_SRC_DIR/crtbrand.c"
        dump "Please see the content of $TMPL for details!"
        cat $TMPL | tail -10
        exit 1
    fi

    for SRC_FILE in $(cd "$SRC_DIR" && ls crt*.[cS]); do
        DST_FILE=${SRC_FILE%%.c}
        DST_FILE=${DST_FILE%%.S}.o

        case "$DST_FILE" in
            "crtend.o")
                # Special case: crtend.S must be compiled as crtend_android.o
                # This is for long historical reasons, i.e. to avoid name conflicts
                # in the past with other crtend.o files. This is hard-coded in the
                # Android toolchain configuration, so switch the name here.
                DST_FILE=crtend_android.o
                ;;
            "crtbegin_dynamic.o"|"crtbegin_static.o")
                # Add .note.ABI-tag section
                SRC_FILE=$SRC_FILE" $CRTBRAND_S"
                ;;
            "crtbegin.o")
                # If we have a single source for both crtbegin_static.o and
                # crtbegin_dynamic.o we generate one and make a copy later.
                DST_FILE=crtbegin_dynamic.o
                # Add .note.ABI-tag section
                SRC_FILE=$SRC_FILE" $CRTBRAND_S"
                ;;
        esac

        log "Generating $ARCH C runtime object: $DST_FILE"
        (cd "$SRC_DIR" && $CC \
                 -I$SRCDIR/../../bionic/libc/include \
                 -I$SRCDIR/../../bionic/libc/private \
                 -I$SRCDIR/../../bionic/libc/arch-$ARCH/include \
                 -isystem $SRCDIR/../../bionic/libc/kernel/common \
                 -isystem $SRCDIR/../../bionic/libc/kernel/common/linux \
                 -O2 -fpic -Wl,-r -nostdlib -nostdinc -o "$DST_DIR/$DST_FILE" $SRC_FILE) 1>>$TMPL 2>&1
        if [ $? != 0 ]; then
            dump "ERROR: Could not generate $DST_FILE from $SRC_DIR/$SRC_FILE"
            dump "Please see the content of $TMPL for details!"
            cat $TMPL | tail -10
            exit 1
        fi
        if [ ! -s "$DST_DIR/crtbegin_static.o" ]; then
            cp "$DST_DIR/crtbegin_dynamic.o" "$DST_DIR/crtbegin_static.o"
        fi
    done
    rm -f "$CRTBRAND_S"
}

# $1: platform number
# $2: architecture
# $3: target NDK directory
generate_api_level ()
{
    local API=$1
    local ARCH=$2
    local HEADER="platforms/android-$API/arch-$ARCH/usr/include/android/api-level.h"
    log "Generating: $HEADER"
    rm -f "$3/$HEADER"  # Remove symlink if any.
    cat > "$3/$HEADER" <<EOF
/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef ANDROID_API_LEVEL_H
#define ANDROID_API_LEVEL_H

#define __ANDROID_API__ $API

#endif /* ANDROID_API_LEVEL_H */
EOF
}

# Copy platform sysroot and samples into your destination
#

# if $SRC/android-$PLATFORM/arch-$ARCH exists
#   $SRC/android-$PLATFORM/include --> $DST/android-$PLATFORM/arch-$ARCH/usr/include
#   $SRC/android-$PLATFORM/arch-$ARCH/include --> $DST/android-$PLATFORM/arch-$ARCH/usr/include
#   $SRC/android-$PLATFORM/arch-$ARCH/lib --> $DST/android-$PLATFORM/arch-$ARCH/usr/lib
#
if [ -z "$OPTION_OVERLAY" ]; then
    rm -rf $DSTDIR/platforms && mkdir -p $DSTDIR/platforms
fi
for ARCH in $ARCHS; do
    # Find first platform for this arch
    PREV_SYSROOT_DST=
    for PLATFORM in $PLATFORMS; do
        PLATFORM_DST=platforms/android-$PLATFORM   # Relative to $DSTDIR
        PLATFORM_SRC=$PLATFORM_DST                 # Relative to $SRCDIR
        SYSROOT_DST=$PLATFORM_DST/arch-$ARCH/usr
        # Skip over if there is no arch-specific file for this platform
        # and no destination platform directory was created. This is needed
        # because x86 and MIPS don't have files for API levels 3-8.
        if [ -z "$PREV_SYSROOT_DST" -a \
           ! -d "$SRCDIR/$PLATFORM_SRC/arch-$ARCH" ]; then
            log "Skipping: \$SRC/$PLATFORM_SRC/arch-$ARCH"
            continue
        fi

        log "Populating \$DST/platforms/android-$PLATFORM/arch-$ARCH"

        # If this is not the first destination directory, copy over, or
        # symlink the files from the previous one now.
        if [ "$PREV_SYSROOT_DST" ]; then
            if [ "$OPTION_FAST_COPY" ]; then
                log "Copying \$DST/$PREV_SYSROOT_DST to \$DST/$SYSROOT_DST"
                copy_directory "$DSTDIR/$PREV_SYSROOT_DST" "$DSTDIR/$SYSROOT_DST"
            else
                log "Symlink-copying \$DST/$PREV_SYSROOT_DST to \$DST/$SYSROOT_DST"
                symlink_src_directory $PREV_SYSROOT_DST $SYSROOT_DST
            fi
        fi

        # If this is the first destination directory, copy the common
        # files from previous platform directories into this one.
        # This helps copy the common headers from android-3 to android-8
        # into the x86 and mips android-9 directories.
        if [ -z "$PREV_SYSROOT_DST" ]; then
            for OLD_PLATFORM in $PLATFORMS; do
                if [ "$OLD_PLATFORM" -eq "$PLATFORM" ]; then
                    break
                fi
                copy_src_directory platforms/android-$OLD_PLATFORM/include \
                                   $SYSROOT_DST/include \
                                   "common android-$OLD_PLATFORM headers"
            done
        fi

        # Now copy over all non-arch specific include files
        copy_src_directory $PLATFORM_SRC/include $SYSROOT_DST/include "common system headers"
        copy_src_directory $PLATFORM_SRC/arch-$ARCH/include $SYSROOT_DST/include "$ARCH system headers"

        generate_api_level "$PLATFORM" "$ARCH" "$DSTDIR"

        # If --minimal is not used, copy or generate binary files.
        if [ -z "$OPTION_MINIMAL" ]; then
            # Copy the prebuilt static libraries.
            if [ "$ARCH" = "x86_64" ]; then
            # We need full set for multilib compiler
                copy_src_directory $PLATFORM_SRC/arch-$ARCH/lib $SYSROOT_DST/lib "x86 sysroot libs"
                copy_src_directory $PLATFORM_SRC/arch-$ARCH/lib64 $SYSROOT_DST/lib64 "x86_64 sysroot libs"
                copy_src_directory $PLATFORM_SRC/arch-$ARCH/libx32 $SYSROOT_DST/libx32 "x32 sysroot libs"
            else
                copy_src_directory $PLATFORM_SRC/arch-$ARCH/lib $SYSROOT_DST/lib "$ARCH sysroot libs"
            fi

            # Generate C runtime object files when available
            PLATFORM_SRC_ARCH=$PLATFORM_SRC/arch-$ARCH/src
            if [ ! -d "$SRCDIR/$PLATFORM_SRC_ARCH" ]; then
                PLATFORM_SRC_ARCH=$PREV_PLATFORM_SRC_ARCH
            else
                PREV_PLATFORM_SRC_ARCH=$PLATFORM_SRC_ARCH
            fi

            # Genreate crt objects for known archs
            if [ "$(arch_in_unknown_archs $ARCH)" != "yes" ]; then
               if [ "$ARCH" = "x86_64" ]; then
               # We need full set for multilib compiler
                 gen_crt_objects $PLATFORM $ARCH platforms/common/src $PLATFORM_SRC_ARCH $SYSROOT_DST/lib "-m32"
                 gen_crt_objects $PLATFORM $ARCH platforms/common/src $PLATFORM_SRC_ARCH $SYSROOT_DST/lib64 "-m64"
                 gen_crt_objects $PLATFORM $ARCH platforms/common/src $PLATFORM_SRC_ARCH $SYSROOT_DST/libx32 "-mx32"
               else
                 gen_crt_objects $PLATFORM $ARCH platforms/common/src $PLATFORM_SRC_ARCH $SYSROOT_DST/lib
               fi
            fi

            # Generate shared libraries from symbol files
            if [ "$ARCH" = "x86_64" ]; then
               # We need full set for multilib compiler
               gen_shared_libraries $ARCH $PLATFORM_SRC/arch-$ARCH/symbols $SYSROOT_DST/lib "-m32"
               gen_shared_libraries $ARCH $PLATFORM_SRC/arch-$ARCH/symbols $SYSROOT_DST/lib64 "-m64"
               gen_shared_libraries $ARCH $PLATFORM_SRC/arch-$ARCH/symbols $SYSROOT_DST/libx32 "-mx32"
            else
               gen_shared_libraries $ARCH $PLATFORM_SRC/arch-$ARCH/symbols $SYSROOT_DST/lib
            fi
        else
            # Copy the prebuilt binaries to bootstrap GCC
            if [ "$ARCH" = "x86_64" ]; then
               # We need full set for multilib compiler
               copy_src_directory $PLATFORM_SRC/arch-$ARCH/lib-bootstrap/lib $SYSROOT_DST/lib "x86 sysroot libs (boostrap)"
               copy_src_directory $PLATFORM_SRC/arch-$ARCH/lib-bootstrap/lib64 $SYSROOT_DST/lib64 "x86_64 sysroot libs (boostrap)"
               copy_src_directory $PLATFORM_SRC/arch-$ARCH/lib-bootstrap/libx32 $SYSROOT_DST/libx32 "x32 sysroot libs (boostrap)"
            else
               copy_src_directory $PLATFORM_SRC/arch-$ARCH/lib-bootstrap $SYSROOT_DST/lib "$ARCH sysroot libs (boostrap)"
            fi
        fi
        PREV_SYSROOT_DST=$SYSROOT_DST
    done
done

#
# $SRC/android-$PLATFORM/samples --> $DST/samples
#
if [ "$OPTION_SAMPLES" ] ; then
    # Copy platform samples and generic samples into your destination
    #
    # $SRC/samples/ --> $DST/samples/
    # $SRC/android-$PLATFORM/samples/ --> $DST/samples
    #
    dump "Copying generic samples"
    if [ -z "$OPTION_OVERLAY" ]; then
        rm -rf $DSTDIR/samples && mkdir -p $DSTDIR/samples
    fi
    copy_src_directory  samples samples samples

    for PLATFORM in $PLATFORMS; do
        dump "Copy android-$PLATFORM samples"
        # $SRC/platform-$PLATFORM/samples --> $DST/samples
        copy_src_directory platforms/android-$PLATFORM/samples samples samples
    done

    # Cleanup generated files in samples
    rm -rf "$DSTDIR/samples/*/obj"
    rm -rf "$DSTDIR/samples/*/libs"
fi

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
    ARCHIVE=platforms.tar.bz2
    dump "Packaging $ARCHIVE"
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$DSTDIR" "platforms"
    fail_panic "Could not package platforms"
    if [ "$OPTION_SAMPLES" ]; then
        ARCHIVE=samples.tar.bz2
        dump "Packaging $ARCHIVE"
        pack_archive "$PACKAGE_DIR/$ARCHIVE" "$DSTDIR" "samples"
        fail_panic "Could not package samples"
    fi
fi

log "Done !"
