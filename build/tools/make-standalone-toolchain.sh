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

# For now, this is the only toolchain that works reliably.
TOOLCHAIN_NAME=
register_var_option "--toolchain=<name>" TOOLCHAIN_NAME "Specify toolchain name"

LLVM_VERSION=
register_var_option "--llvm-version=<ver>" LLVM_VERSION "Specify LLVM version"

STL=gnustl
register_var_option "--stl=<name>" STL "Specify C++ STL"

ARCH=
register_var_option "--arch=<name>" ARCH "Specify target architecture"

# Grab the ABIs that match the architecture.
ABIS=
register_var_option "--abis=<list>" ABIS "Specify list of target ABIs."

NDK_DIR=`dirname $0`
NDK_DIR=`dirname $NDK_DIR`
NDK_DIR=`dirname $NDK_DIR`
register_var_option "--ndk-dir=<path>" NDK_DIR "Take source files from NDK at <path>"

if [ -d "$NDK_DIR/prebuilt/$HOST_TAG" ]; then
  SYSTEM=$HOST_TAG
else
  SYSTEM=$HOST_TAG32
fi
register_var_option "--system=<name>" SYSTEM "Specify host system"

PACKAGE_DIR=/tmp/ndk-$USER
register_var_option "--package-dir=<path>" PACKAGE_DIR "Place package file in <path>"

INSTALL_DIR=
register_var_option "--install-dir=<path>" INSTALL_DIR "Don't create package, install files to <path> instead."

PLATFORM=
register_option "--platform=<name>" do_platform "Specify target Android platform/API level." "android-3"
do_platform () {
    PLATFORM=$1;
    if [ "$PLATFORM" = "android-L" ]; then
        echo "WARNING: android-L is renamed as android-21"
        PLATFORM=android-21
    fi
}

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
        mipsel-*)
            ARCH=mips
            ;;
        aarch64-*)
            ARCH=arm64
            ;;
        x86_64-linux-android-*)
            ARCH=x86_64
            TOOLCHAIN_NAME=$(echo "$TOOLCHAIN_NAME" | sed -e 's/-linux-android//')
            echo "Auto-truncate: --toolchain=$TOOLCHAIN_NAME"
            ;;
        x86_64-*)
            ARCH=x86_64
            ;;
        mips64el-*)
            ARCH=mips64
            ;;
        *)
            ARCH=null
            echo "Unable to auto-config arch from toolchain $TOOLCHAIN_NAME"
            exit 1
            ;;
    esac
    ARCH_INC=$ARCH
    log "Auto-config: --arch=$ARCH"
else
    ARCH_INC=$ARCH
    case $ARCH in
        *arm)
            ARCH=arm
            ;;
        *x86)
            ARCH=x86
            ;;
        *mips)
            ARCH=mips
            ;;
        *arm64)
            ARCH=arm64
            ;;
        *x86_64)
            ARCH=x86_64
            ;;
        *mips64)
            ARCH=mips64
            ;;
        *)
            echo "Invalid --arch $ARCH"
            echo "Please use one of arm, x86, mips, arm64, x86_64 or mips64"
            ARCH=null
            exit 1
            ;;
    esac

fi

if [ -z "$ABIS" ]; then
    ABIS=$(convert_arch_to_abi $ARCH)
fi

if [ -z "$ABIS" ]; then
    dump "ERROR: No ABIS. Possibly unsupported NDK architecture $ARCH?"
    exit 1
fi

ARCH_LIB=$ARCH
ARCH_STL=$ARCH
if [ "$ARCH_INC" != "$ARCH" ]; then
    test -n "`echo $ARCH_INC | grep bc$ARCH`" && NEED_BC2NATIVE=yes
    test -z "`echo $ARCH_INC | grep $ARCH`" && NEED_BC_LIB=yes
    ARCH_INC=$(find_ndk_unknown_archs)
    test -z "$ARCH_INC" && ARCH_INC="$ARCH"
    test "$NEED_BC_LIB" = "yes" && ARCH_LIB=$ARCH_INC
    test "$NEED_BC_LIB" = "yes" -o "$NEED_BC2NATIVE" = "yes" && ARCH_STL=$ARCH_INC
fi

# Check toolchain name
if [ -z "$TOOLCHAIN_NAME" ]; then
    TOOLCHAIN_NAME=$(get_default_toolchain_name_for_arch $ARCH)
    echo "Auto-config: --toolchain=$TOOLCHAIN_NAME"
fi

if [ "$ARCH_STL" != "$ARCH" ]; then
    if [ "$STL" != stlport ]; then
        echo "Force-config: --stl=stlport"
        STL=stlport
    fi
fi

if [ "$ARCH_INC" != "$ARCH" ]; then
    TARGET_ABI=$(convert_arch_to_abi $ARCH | tr ',' '\n' | tail -n 1)
    if [ -z "$LLVM_VERSION" ]; then
        LLVM_VERSION=$DEFAULT_LLVM_VERSION
    fi
fi

# Detect LLVM version from toolchain name with *clang*
LLVM_VERSION_EXTRACT=$(echo "$TOOLCHAIN_NAME" | grep 'clang[0-9]\.[0-9]$' | sed -e 's/.*-clang//')
if [ -n "$LLVM_VERSION_EXTRACT" ]; then
    DEFAULT_GCC_VERSION=$(get_default_gcc_version_for_arch $ARCH)
    NEW_TOOLCHAIN_NAME=${TOOLCHAIN_NAME%-clang${LLVM_VERSION_EXTRACT}}-${DEFAULT_GCC_VERSION}
    if [ -z "$LLVM_VERSION" ]; then
        LLVM_VERSION=$LLVM_VERSION_EXTRACT
        echo "Auto-config: --toolchain=$NEW_TOOLCHAIN_NAME, --llvm-version=$LLVM_VERSION"
    else
        if [ "$LLVM_VERSION" != "$LLVM_VERSION_EXTRACT" ]; then
            echo "Conflict llvm-version: --llvm-version=$LLVM_VERSION and as implied by --toolchain=$TOOLCHAIN_NAME"
            exit 1
	fi
    fi
    TOOLCHAIN_NAME=$NEW_TOOLCHAIN_NAME
fi

# Check PLATFORM
if [ -z "$PLATFORM" -a "$ARCH_INC" = "$ARCH" ] ; then
    case $ARCH in
        arm) PLATFORM=android-3
            ;;
        x86|mips)
            PLATFORM=android-9
            ;;
        arm64|x86_64|mips64)
            PLATFORM=android-$FIRST_API64_LEVEL
            ;;
        *)
            dump "ERROR: Unsupported NDK architecture $ARCH!"
    esac
    log "Auto-config: --platform=$PLATFORM"
elif [ -z "$PLATFORM" ] ; then
    PLATFORM=android-9
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
parse_toolchain_name $TOOLCHAIN_NAME

case "$TOOLCHAIN_NAME" in
  *4.9l)
    GCC_VERSION=4.9l
    ;;
  *4.8l)
    GCC_VERSION=4.8l
    ;;
esac

# Check that there are any platform files for it!
(cd $NDK_DIR/platforms && ls -d */arch-$ARCH_INC >/dev/null 2>&1 )
if [ $? != 0 ] ; then
    echo "Platform $PLATFORM doesn't have any files for this architecture: $ARCH_INC"
    echo "Either use --platform=<name> or --toolchain=<name> to select a different"
    echo "platform or arch-dependent toolchain name (respectively)!"
    exit 1
fi

# Compute source sysroot
SRC_SYSROOT_INC="$NDK_DIR/platforms/$PLATFORM/arch-$ARCH_INC/usr/include"
SRC_SYSROOT_LIB="$NDK_DIR/platforms/$PLATFORM/arch-$ARCH_LIB/usr/lib"
if [ ! -d "$SRC_SYSROOT_INC" -o ! -d "$SRC_SYSROOT_LIB" ] ; then
    echo "No platform files ($PLATFORM) for this architecture: $ARCH"
    exit 1
fi

# Check that we have any prebuilts GCC toolchain here
if [ ! -d "$TOOLCHAIN_PATH/prebuilt" ]; then
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
TOOLCHAIN_GCC=$TOOLCHAIN_PATH/bin/$ABI_CONFIGURE_TARGET-gcc

if [ ! -f "$TOOLCHAIN_GCC" ] ; then
    echo "Toolchain $TOOLCHAIN_GCC is missing!"
    exit 1
fi

if [ -n "$LLVM_VERSION" ]; then
    LLVM_TOOLCHAIN_PATH="$NDK_DIR/toolchains/llvm-$LLVM_VERSION"
    # Check that we have any prebuilts LLVM toolchain here
    if [ ! -d "$LLVM_TOOLCHAIN_PATH/prebuilt" ] ; then
        echo "LLVM Toolchain is missing prebuilt files"
        echo "You must point to a valid NDK release package!"
        exit 1
    fi

    if [ ! -d "$LLVM_TOOLCHAIN_PATH/prebuilt/$SYSTEM" ] ; then
        echo "Host system '$SYSTEM' is not supported by the source NDK!"
        echo "Try --system=<name> with one of: " `(cd $LLVM_TOOLCHAIN_PATH/prebuilt && ls)`
        exit 1
    fi
    LLVM_TOOLCHAIN_PATH="$LLVM_TOOLCHAIN_PATH/prebuilt/$SYSTEM"
fi

# Get GCC_BASE_VERSION.  Note that GCC_BASE_VERSION may be slightly different from GCC_VERSION.
# eg. In gcc4.9 GCC_BASE_VERSION is "4.9.x-google"
LIBGCC_PATH=`$TOOLCHAIN_GCC -print-libgcc-file-name`
LIBGCC_BASE_PATH=${LIBGCC_PATH%/*}         # base path of libgcc.a
GCC_BASE_VERSION=${LIBGCC_BASE_PATH##*/}   # stuff after the last /

# Create temporary directory
TMPDIR=$NDK_TMPDIR/standalone/$TOOLCHAIN_NAME

dump "Copying prebuilt binaries..."
# Now copy the GCC toolchain prebuilt binaries
copy_directory "$TOOLCHAIN_PATH" "$TMPDIR"

# Replace soft-link mcld by real file
ALL_LDS=`find $TMPDIR -name "*mcld"`
for LD in $ALL_LDS; do
  rm -f "$LD"
  cp -a "$NDK_DIR/toolchains/llvm-$DEFAULT_LLVM_VERSION/prebuilt/$SYSTEM/bin/ld.mcld" "$LD"
done

# Copy python-related to for gdb.exe
PYTHON=python
PYTHON_x=python$(echo "$DEFAULT_PYTHON_VERSION" | cut -d . -f 1)
PYTHON_xdotx=python$(echo "$DEFAULT_PYTHON_VERSION" | cut -d . -f 1-2)
copy_directory "$NDK_DIR/prebuilt/$SYSTEM/include/$PYTHON_xdotx" "$TMPDIR/include/$PYTHON_xdotx"
copy_directory "$NDK_DIR/prebuilt/$SYSTEM/lib/$PYTHON_xdotx" "$TMPDIR/lib/$PYTHON_xdotx"
copy_file_list "$NDK_DIR/prebuilt/$SYSTEM/bin" "$TMPDIR/bin" "$PYTHON$HOST_EXE" "$PYTHON_x$HOST_EXE" "$PYTHON_xdotx$HOST_EXE"
if [ "$HOST_TAG32" = "windows" ]; then
  copy_file_list "$NDK_DIR/prebuilt/$SYSTEM/bin" "$TMPDIR/bin" lib$PYTHON_xdotx.dll
fi

# Copy yasm for x86
if [ "$ARCH" = "x86" ]; then
  copy_file_list "$NDK_DIR/prebuilt/$SYSTEM/bin" "$TMPDIR/bin" "yasm$HOST_EXE"
fi

# Clang stuff

dump_extra_compile_commands () {
  if [ "$NEED_BC2NATIVE" != "yes" ]; then
    return
  fi

  if [ -z "$HOST_EXE" ]; then
    echo '# Call bc2native if needed'
    echo ''
    echo 'if [ -n "`echo $@ | grep '\'\\ \\-c\''`" ] || [ "$1" = "-c" ]; then'
    echo '  exit'
    echo 'fi'

    echo 'while [ -n "$1" ]; do'
    echo '  if [ "$1" = "-o" ]; then'
    echo '    output="$2"'
    echo '    break'
    echo '  fi'
    echo '  shift'
    echo 'done'
    echo 'test -z "$output" && output=a.out'
    echo 'if [ -f "`dirname $0`/ndk-bc2native" ]; then'
    echo '  `dirname $0`/ndk-bc2native --sysroot=`dirname $0`/../sysroot --abi='$TARGET_ABI' --platform='$PLATFORM' --file $output $output'
    echo 'else'
    echo '  export PYTHONPATH=`dirname $0`/../lib/python2.7/'
    echo '  `dirname $0`/python `dirname $0`/ndk-bc2native.py --sysroot=`dirname $0`/../sysroot --abi='$TARGET_ABI' --platform='$PLATFORM' --file $output $output'
    echo 'fi'
  else
    echo 'rem Call bc2native if needed'
    echo ''
    echo '  if not "%1" == "-c" goto :keep_going'
    echo '  echo %* | grep "\\ \\-c"'
    echo '  if ERRORLEVEL 1 goto :keep_going'
    echo '  exit'
    echo ':keep_going'

    echo ':keep_find_output'
    echo '  if not "%1" == "-o" goto :check_next'
    echo '  set output=%2'
    echo ':check_next'
    echo '  shift'
    echo '  if "%1" == "" goto :keep_find_output'
    echo '  if not "%output%" == "" goto :check_done'
    echo '  set output=a.out'
    echo ':check_done'
    echo 'if exist %~dp0\\ndk-bc2native'$HOST_EXE' ('
    echo '  %~dp0\\ndk-bc2native'$HOST_EXE' --sysroot=%~dp0\\.\\sysroot --abi='$TARGET_ABI' --platform='$PLATFORM' --file %output% %output'
    echo 'else ('
    echo '  set PYTHONPATH=%~dp0\\..\\lib\\python2.7\\'
    echo '  %~dp0\\python'$HOST_EXE' %~dp0\\ndk-bc2native.py --sysroot=%~dp0\\..\\sysroot --abi='$TARGET_ABI' --platform='$PLATFORM' --file %output% %output%'
    echo ')'
  fi
}

if [ -n "$LLVM_VERSION" ]; then
  # Copy the clang/llvm toolchain prebuilt binaries
  copy_directory "$LLVM_TOOLCHAIN_PATH" "$TMPDIR"

  # Move clang and clang++ to clang${LLVM_VERSION} and clang${LLVM_VERSION}++,
  # then create scripts linking them with predefined -target flag.  This is to
  # make clang/++ easier drop-in replacement for gcc/++ in NDK standalone mode.
  # Note that the file name of "clang" isn't important, and the trailing
  # "++" tells clang to compile in C++ mode
  LLVM_TARGET=
  case "$ARCH" in
      arm) # NOte: -target may change by clang based on the
           #        presence of subsequent -march=armv7-a and/or -mthumb
          LLVM_TARGET=armv5te-none-linux-androideabi
          TOOLCHAIN_PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm
          ;;
      x86)
          LLVM_TARGET=i686-none-linux-android
          TOOLCHAIN_PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86
          ;;
      mips)
          LLVM_TARGET=mipsel-none-linux-android
          TOOLCHAIN_PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_mips
          ;;
      arm64)
          LLVM_TARGET=aarch64-none-linux-android
          TOOLCHAIN_PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm64
          ;;
      x86_64)
          LLVM_TARGET=x86_64-none-linux-android
          TOOLCHAIN_PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86_64
          ;;
      mips64)
          LLVM_TARGET=mips64el-none-linux-android
          TOOLCHAIN_PREFIX=$DEFAULT_ARCH_TOOLCHAIN_PREFIX_mips64
          ;;
      *)
        dump "ERROR: Unsupported NDK architecture $ARCH!"
  esac
  # Need to remove '.' from LLVM_VERSION when constructing new clang name,
  # otherwise clang3.3++ may still compile *.c code as C, not C++, which
  # is not consistent with g++
  LLVM_VERSION_WITHOUT_DOT=$(echo "$LLVM_VERSION" | sed -e "s!\.!!")
  mv "$TMPDIR/bin/clang${HOST_EXE}" "$TMPDIR/bin/clang${LLVM_VERSION_WITHOUT_DOT}${HOST_EXE}"
  if [ -h "$TMPDIR/bin/clang++${HOST_EXE}" ] ; then
    ## clang++ is a link to clang.  Remove it and reconstruct
    rm "$TMPDIR/bin/clang++${HOST_EXE}"
    ln -sf "clang${LLVM_VERSION_WITHOUT_DOT}${HOST_EXE}" "$TMPDIR/bin/clang${LLVM_VERSION_WITHOUT_DOT}++${HOST_EXE}"
  else
    mv "$TMPDIR/bin/clang++${HOST_EXE}" "$TMPDIR/bin/clang$LLVM_VERSION_WITHOUT_DOT++${HOST_EXE}"
  fi

  EXTRA_CLANG_FLAGS=
  EXTRA_CLANGXX_FLAGS=
  if [ "$ARCH_STL" != "$ARCH" ]; then
    LLVM_TARGET=le32-none-ndk
    EXTRA_CLANG_FLAGS="-emit-llvm"
    EXTRA_CLANGXX_FLAGS="$EXTRA_CLANG_FLAGS -I\`dirname \$0\`/../include/c++/$GCC_BASE_VERSION"
  fi

  cat > "$TMPDIR/bin/clang" <<EOF
if [ "\$1" != "-cc1" ]; then
    \`dirname \$0\`/clang$LLVM_VERSION_WITHOUT_DOT -target $LLVM_TARGET "\$@" $EXTRA_CLANG_FLAGS
    $(dump_extra_compile_commands)
else
    # target/triple already spelled out.
    \`dirname \$0\`/clang$LLVM_VERSION_WITHOUT_DOT "\$@" $EXTRA_CLANG_FLAGS
fi
EOF
  cat > "$TMPDIR/bin/clang++" <<EOF
if [ "\$1" != "-cc1" ]; then
    \`dirname \$0\`/clang$LLVM_VERSION_WITHOUT_DOT++ -target $LLVM_TARGET "\$@" $EXTRA_CLANGXX_FLAGS
    $(dump_extra_compile_commands)
else
    # target/triple already spelled out.
    \`dirname \$0\`/clang$LLVM_VERSION_WITHOUT_DOT++ "\$@" $EXTRA_CLANGXX_FLAGS
fi
EOF
  chmod 0755 "$TMPDIR/bin/clang" "$TMPDIR/bin/clang++"
  cp -a "$TMPDIR/bin/clang" "$TMPDIR/bin/$TOOLCHAIN_PREFIX-clang"
  cp -a "$TMPDIR/bin/clang++" "$TMPDIR/bin/$TOOLCHAIN_PREFIX-clang++"

  if [ -n "$HOST_EXE" ] ; then
    cat > "$TMPDIR/bin/clang.cmd" <<EOF
@echo off
if "%1" == "-cc1" goto :L
%~dp0\\clang${LLVM_VERSION_WITHOUT_DOT}${HOST_EXE} -target $LLVM_TARGET %* $EXTRA_CLANG_FLAGS
$(dump_extra_compile_commands)
if ERRORLEVEL 1 exit /b 1
goto :done
:L
rem target/triple already spelled out.
%~dp0\\clang${LLVM_VERSION_WITHOUT_DOT}${HOST_EXE} %* $EXTRA_CLANG_FLAGS
if ERRORLEVEL 1 exit /b 1
:done
EOF
    cat > "$TMPDIR/bin/clang++.cmd" <<EOF
@echo off
if "%1" == "-cc1" goto :L
%~dp0\\clang${LLVM_VERSION_WITHOUT_DOT}++${HOST_EXE} -target $LLVM_TARGET %* $EXTRA_CLANGXX_FLAGS
$(dump_extra_compile_commands)
if ERRORLEVEL 1 exit /b 1
goto :done
:L
rem target/triple already spelled out.
%~dp0\\clang${LLVM_VERSION_WITHOUT_DOT}++${HOST_EXE} %* $EXTRA_CLANGXX_FLAGS
if ERRORLEVEL 1 exit /b 1
:done
EOF
    chmod 0755 "$TMPDIR/bin/clang.cmd" "$TMPDIR/bin/clang++.cmd"
    cp -a "$TMPDIR/bin/clang.cmd" "$TMPDIR/bin/$TOOLCHAIN_PREFIX-clang.cmd"
    cp -a "$TMPDIR/bin/clang++.cmd" "$TMPDIR/bin/$TOOLCHAIN_PREFIX-clang++.cmd"
  fi
fi

dump "Copying sysroot headers and libraries..."
# Copy the sysroot under $TMPDIR/sysroot. The toolchain was built to
# expect the sysroot files to be placed there!
copy_directory_nolinks "$SRC_SYSROOT_INC" "$TMPDIR/sysroot/usr/include"
copy_directory_nolinks "$SRC_SYSROOT_LIB" "$TMPDIR/sysroot/usr/lib"
case "$ARCH" in
# x86_64 and mips* toolchain are built multilib.
    x86_64)
        copy_directory_nolinks "$SRC_SYSROOT_LIB/../lib64" "$TMPDIR/sysroot/usr/lib64"
        copy_directory_nolinks "$SRC_SYSROOT_LIB/../libx32" "$TMPDIR/sysroot/usr/libx32"
        ;;
    mips64)
        copy_directory_nolinks "$SRC_SYSROOT_LIB/../libr2" "$TMPDIR/sysroot/usr/libr2"
        copy_directory_nolinks "$SRC_SYSROOT_LIB/../libr6" "$TMPDIR/sysroot/usr/libr6"
        copy_directory_nolinks "$SRC_SYSROOT_LIB/../lib64" "$TMPDIR/sysroot/usr/lib64"
        copy_directory_nolinks "$SRC_SYSROOT_LIB/../lib64r2" "$TMPDIR/sysroot/usr/lib64r2"
        ;;
    mips)
        if [ "$GCC_VERSION" = "4.9" ]; then
            copy_directory_nolinks "$SRC_SYSROOT_LIB/../libr2" "$TMPDIR/sysroot/usr/libr2"
            copy_directory_nolinks "$SRC_SYSROOT_LIB/../libr6" "$TMPDIR/sysroot/usr/libr6"
	fi
        ;;
esac

if [ "$ARCH_INC" != "$ARCH" ]; then
    cp -a $NDK_DIR/$GABIXX_SUBDIR/libs/$ABI/* $TMPDIR/sysroot/usr/lib
    cp -a $NDK_DIR/$LIBPORTABLE_SUBDIR/libs/$ABI/* $TMPDIR/sysroot/usr/lib
    cp -a $NDK_DIR/$GCCUNWIND_SUBDIR/libs/$ABI/* $TMPDIR/sysroot/usr/lib
    if [ "$ARCH" = "${ARCH%%64*}" ]; then
        cp -a $NDK_DIR/$COMPILER_RT_SUBDIR/libs/$ABI/* $TMPDIR/sysroot/usr/lib
    fi
fi

if [ "$ARCH_LIB" != "$ARCH" ]; then
    cp -a $NDK_DIR/platforms/$PLATFORM/arch-$ARCH/usr/lib/crt* $TMPDIR/sysroot/usr/lib
fi

GNUSTL_DIR=$NDK_DIR/$GNUSTL_SUBDIR/$GCC_VERSION
GNUSTL_LIBS=$GNUSTL_DIR/libs

STLPORT_DIR=$NDK_DIR/$STLPORT_SUBDIR
STLPORT_LIBS=$STLPORT_DIR/libs

LIBCXX_DIR=$NDK_DIR/$LIBCXX_SUBDIR
LIBCXX_LIBS=$LIBCXX_DIR/libs
case $ARCH in
    x86|x86_64|mips|mips64)
        LIBCXX_SUPPORT_LIB=gabi++
        ;;
    *)
        LIBCXX_SUPPORT_LIB=libc++abi
        ;;
esac

SUPPORT_DIR=$NDK_DIR/$SUPPORT_SUBDIR

COMPILER_RT_DIR=$NDK_DIR/$COMPILER_RT_SUBDIR
COMPILER_RT_LIBS=$COMPILER_RT_DIR/libs

if [ "$STL" = "libcxx" -o "$STL" = "libc++" ]; then
    dump "Copying c++ runtime headers and libraries (with $LIBCXX_SUPPORT_LIB)..."
else
    dump "Copying c++ runtime headers and libraries..."
fi

ABI_STL="$TMPDIR/$ABI_CONFIGURE_TARGET"
ABI_STL_INCLUDE="$TMPDIR/include/c++/$GCC_BASE_VERSION"
ABI_STL_INCLUDE_TARGET="$ABI_STL_INCLUDE/$ABI_CONFIGURE_TARGET"

# $1: filenames of headers
copy_abi_headers () {
  local ABI_NAME=$1
  shift

  for header in $@; do
    (set -e; cd $ABI_STL_INCLUDE && cp -a ../../$ABI_NAME/include/$header $header) || exit 1
  done
}

# Copy common STL headers (i.e. the non-arch-specific ones)
copy_stl_common_headers () {
    case $STL in
        gnustl)
            copy_directory "$GNUSTL_DIR/include" "$ABI_STL_INCLUDE"
            ;;
        libcxx|libc++)
            copy_directory "$LIBCXX_DIR/libcxx/include" "$ABI_STL_INCLUDE"
            copy_directory "$SUPPORT_DIR/include" "$ABI_STL_INCLUDE"
            if [ "$LIBCXX_SUPPORT_LIB" = "gabi++" ]; then
                copy_directory "$STLPORT_DIR/../gabi++/include" "$ABI_STL_INCLUDE/../../gabi++/include"
                copy_abi_headers gabi++ cxxabi.h unwind.h unwind-arm.h unwind-itanium.h gabixx_config.h
            elif [ "$LIBCXX_SUPPORT_LIB" = "libc++abi" ]; then
                copy_directory "$LIBCXX_DIR/../llvm-libc++abi/libcxxabi/include" "$ABI_STL_INCLUDE/../../llvm-libc++abi/include"
                copy_abi_headers llvm-libc++abi cxxabi.h libunwind.h unwind.h
            else
                dump "ERROR: Unknown libc++ support lib: $LIBCXX_SUPPORT_LIB"
                exit 1
            fi
            ;;
        stlport)
            copy_directory "$STLPORT_DIR/stlport" "$ABI_STL_INCLUDE"
            copy_directory "$STLPORT_DIR/../gabi++/include" "$ABI_STL_INCLUDE/../../gabi++/include"
            copy_abi_headers gabi++ cxxabi.h unwind.h unwind-arm.h unwind-itanium.h gabixx_config.h
            ;;
    esac
}

# $1: Source ABI (e.g. 'armeabi')
# #2  Optional destination path of additional header to copy (eg. include/bits), default to empty
# $3: Optional source path of additional additional header to copy, default to empty
# $4: Optional destination directory, default to empty (e.g. "", "thumb", "armv7-a/thumb")
# $5: Optional source directory, default to empty (e.g. "", "thumb", "armv7-a/thumb")
copy_stl_libs () {
    local ABI=$1
    local HEADER_DST=$2
    local HEADER_SRC=$3
    local DEST_DIR=$4
    local SRC_DIR=$5
    local ABI_SRC_DIR=$ABI

    if [ -n "$SRC_DIR" ]; then
        ABI_SRC_DIR=$ABI/$SRC_DIR
    else
        if [ "$DEST_DIR" != "${DEST_DIR%%/*}" ] ; then
            ABI_SRC_DIR=$ABI/`basename $DEST_DIR`
        fi
    fi

    case $STL in
        gnustl)
            if [ "$HEADER_SRC" != "" ]; then
                copy_directory "$GNUSTL_LIBS/$ABI/include/$HEADER_SRC" "$ABI_STL_INCLUDE_TARGET/$HEADER_DST"
            fi
            copy_file_list "$GNUSTL_LIBS/$ABI_SRC_DIR" "$ABI_STL/lib/$DEST_DIR" "libgnustl_shared.so"
            copy_file_list "$GNUSTL_LIBS/$ABI_SRC_DIR" "$ABI_STL/lib/$DEST_DIR" "libsupc++.a"
            cp -p "$GNUSTL_LIBS/$ABI_SRC_DIR/libgnustl_static.a" "$ABI_STL/lib/$DEST_DIR/libstdc++.a"
            ;;
        libcxx|libc++)
            if [ "$ARCH" = "${ARCH%%64*}" ]; then
                copy_file_list "$COMPILER_RT_LIBS/$ABI" "$ABI_STL/lib/$DEST_DIR" "libcompiler_rt_shared.so" "libcompiler_rt_static.a"
            fi
            copy_file_list "$LIBCXX_LIBS/$ABI_SRC_DIR" "$ABI_STL/lib/$DEST_DIR" "libc++_shared.so"
            cp -p "$LIBCXX_LIBS/$ABI_SRC_DIR/libc++_static.a" "$ABI_STL/lib/$DEST_DIR/libstdc++.a"
            ;;
        stlport)
            if [ "$ARCH_STL" != "$ARCH" ]; then
              tmp_lib_dir=$TMPDIR/stl
              $NDK_DIR/build/tools/build-cxx-stl.sh --stl=stlport --out-dir=$tmp_lib_dir --abis=unknown
              cp -p "`ls $tmp_lib_dir/sources/cxx-stl/stlport/libs/*/libstlport_static.a`" "$ABI_STL/lib/$DEST_DIR/libstdc++.a"
              cp -p "`ls $tmp_lib_dir/sources/cxx-stl/stlport/libs/*/libstlport_shared.bc`" "$ABI_STL/lib/$DEST_DIR/libstlport_shared.so"
              rm -rf $tmp_lib_dir
            else
              copy_file_list "$STLPORT_LIBS/$ABI_SRC_DIR" "$ABI_STL/lib/$DEST_DIR" "libstlport_shared.so"
              cp -p "$STLPORT_LIBS/$ABI_SRC_DIR/libstlport_static.a" "$ABI_STL/lib/$DEST_DIR/libstdc++.a"
            fi
            ;;
        *)
            dump "ERROR: Unsupported STL: $STL"
            exit 1
            ;;
    esac
}

# $1: Source ABI (e.g. 'armeabi')
copy_stl_libs_for_abi () {
    local ABI=$1

    if [ "$(convert_abi_to_arch "$ABI")" != "$ARCH" ]; then
        dump "ERROR: ABI '$ABI' does not match ARCH '$ARCH'"
        exit 1
    fi

    case $ABI in
        armeabi)
            copy_stl_libs armeabi          "bits"                "bits"
            copy_stl_libs armeabi          "thumb/bits"          "bits"       "/thumb"
            ;;
        armeabi-v7a)
            copy_stl_libs armeabi-v7a      "armv7-a/bits"        "bits"       "armv7-a"
            copy_stl_libs armeabi-v7a      "armv7-a/thumb/bits"  "bits"       "armv7-a/thumb"
            ;;
        armeabi-v7a-hard)
            copy_stl_libs armeabi-v7a-hard ""                    ""           "armv7-a/hard"       "."
            copy_stl_libs armeabi-v7a-hard ""                    ""           "armv7-a/thumb/hard" "thumb"
            ;;
        x86_64)
            if [ "$STL" = "gnustl" ]; then
                copy_stl_libs x86_64       "32/bits"             "32/bits"    ""                   "lib"
                copy_stl_libs x86_64       "bits"                "bits"       "../lib64"           "lib64"
                copy_stl_libs x86_64       "x32/bits"            "x32/bits"   "../libx32"          "libx32"
            else
                copy_stl_libs "$ABI"
            fi
            ;;
        mips64)
            if [ "$STL" = "gnustl" ]; then
                copy_stl_libs mips64       "32/mips-r1/bits"     "32/mips-r1/bits"  ""             "lib"
                copy_stl_libs mips64       "32/mips-r2/bits"     "32/mips-r2/bits"  "../libr2"     "libr2"
                copy_stl_libs mips64       "32/mips-r6/bits"     "32/mips-r6/bits"  "../libr6"     "libr6"
                copy_stl_libs mips64       "bits"                "bits"             "../lib64"     "lib64"
                copy_stl_libs mips64       "mips64-r2/bits"      "mips64-r2/bits"   "../lib64r2"   "lib64r2"
            else
                copy_stl_libs "$ABI"
            fi
            ;;
        mips|mips32r6)
            if [ "$STL" = "gnustl" -a "$GCC_VERSION" = "4.9" ]; then
                copy_stl_libs mips         "bits"                "bits"             "../lib"       "lib"
                copy_stl_libs mips         "mips-r2/bits"        "mips-r2/bits"     "../libr2"     "libr2"
                copy_stl_libs mips         "mips-r6/bits"        "mips-r6/bits"     "../libr6"     "libr6"
            else
                copy_stl_libs mips         "bits"                "bits"
            fi
            ;;
        *)
            copy_stl_libs "$ABI"           "bits"                "bits"
            ;;
    esac
}

mkdir -p "$ABI_STL_INCLUDE_TARGET"
fail_panic "Can't create directory: $ABI_STL_INCLUDE_TARGET"
copy_stl_common_headers
for ABI in $(echo "$ABIS" | tr ',' ' '); do
  copy_stl_libs_for_abi "$ABI"
done

# Install or Package
if [ -n "$INSTALL_DIR" ] ; then
    dump "Copying files to: $INSTALL_DIR"
    if [ ! -d "$INSTALL_DIR" ]; then
        move_directory "$TMPDIR" "$INSTALL_DIR"
    else
        copy_directory "$TMPDIR" "$INSTALL_DIR"
    fi
else
    PACKAGE_FILE="$PACKAGE_DIR/$TOOLCHAIN_NAME.tar.bz2"
    dump "Creating package file: $PACKAGE_FILE"
    pack_archive "$PACKAGE_FILE" "`dirname $TMPDIR`" "$TOOLCHAIN_NAME"
    fail_panic "Could not create tarball from $TMPDIR"
fi
dump "Cleaning up..."
rm -rf $TMPDIR

dump "Done."
