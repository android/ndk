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
#  This shell script is used to rebuild the llvm and clang binaries
#  for the Android NDK.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <ndk-dir> <toolchain>"

PROGRAM_DESCRIPTION=\
"Rebuild the LLVM prebuilt binaries for the Android NDK.

Where <src-dir> is the location of toolchain sources, <ndk-dir> is
the top-level NDK installation path and <toolchain> is the name of
the toolchain to use (e.g. llvm-3.5)."

RELEASE=`date +%Y%m%d`
BUILD_OUT=/tmp/ndk-$USER/build/toolchain
OPTION_BUILD_OUT=
register_var_option "--build-out=<path>" OPTION_BUILD_OUT "Set temporary build directory"

# Note: platform API level 9 or higher is needed for proper C++ support
register_var_option "--platform=<name>"  PLATFORM "Specify platform name"

GMP_VERSION=$DEFAULT_GMP_VERSION
register_var_option "--gmp-version=<version>" GMP_VERSION "Specify gmp version"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Create archive tarball in specific directory"

POLLY=no
do_polly_option () { POLLY=yes; }
register_option "--with-polly" do_polly_option "Enable Polyhedral optimizations for LLVM"

MCLINKER=no
do_mclinker_option () { MCLINKER=yes; }
register_option "--mclinker" do_mclinker_option "Build mclinker as well"

CHECK=no
do_check_option () { CHECK=yes; }
register_option "--check" do_check_option "Check LLVM"

USE_PYTHON=no
do_use_python_option () { USE_PYTHON=yes; }
register_option "--use-python" do_use_python_option "Use python bc2native instead of integrated one"

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

    if [ ! -d "$SRC_DIR/$TOOLCHAIN/llvm" ] ; then
        echo "ERROR: Source directory does not contain llvm sources: $SRC_DIR/$TOOLCHAIN/llvm"
        exit 1
    fi

    if [ -e "$SRC_DIR/$TOOLCHAIN/llvm/tools/polly" -a ! -h "$SRC_DIR/$TOOLCHAIN/llvm/tools/polly" ] ; then
        echo "ERROR: polly, if exist, needs to be a symbolic link: $SRC_DIR/$TOOLCHAIN/llvm/tools/polly"
        exit 1
    fi

    GMP_SOURCE=$SRC_DIR/gmp/gmp-$GMP_VERSION.tar.bz2
    if [ ! -f "$GMP_SOURCE" ] ; then
        echo "ERROR: Source directory does not contain gmp: $GMP_SOURCE"
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

if [ -z "$PLATFORM" ]; then
   PLATFORM="android-"$(get_default_api_level_for_arch $ARCH)
fi

prepare_target_build

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
fi

set_toolchain_ndk $NDK_DIR $TOOLCHAIN

if [ "$MINGW" != "yes" -a "$DARWIN" != "yes" ] ; then
    dump "Using C compiler: $CC"
    dump "Using C++ compiler: $CXX"
fi

if [ "$MINGW" = "yes" -a "$TRY64" != "yes" ]; then
    # Clang3.5 and later needs gcc4.7+ to build, and some of
    # cross toolchain "i586-*" we search for in find_mingw_toolchain()
    # can no longer build.  One solution is to provide DEBIAN_NAME=mingw32
    # BINPREFIX=i686-pc-mingw32msvc- MINGW_GCC=/path/to/i686-w64-mingw32,
    # but ABI_CONFIGURE_HOST is still hard-coded to i586-pc-mingw32msvc.
    # Fixup ABI_CONFIGURE_HOST in this case.
    if [ "$ABI_CONFIGURE_HOST" = "i586-pc-mingw32msvc" ]; then
        MINGW_GCC_BASENAME=`basename $MINGW_GCC`
        if [ "$MINGW_GCC_BASENAME" = "${MINGW_GCC_BASENAME%%i585*}" ]; then
            ABI_CONFIGURE_HOST=${MINGW_GCC_BASENAME%-gcc}
	    STRIP=$ABI_CONFIGURE_HOST-strip
        fi
    fi
fi

rm -rf $BUILD_OUT
mkdir -p $BUILD_OUT

MAKE_FLAGS=
if [ "$VERBOSE" = "yes" ]; then
    MAKE_FLAGS="VERBOSE=1"
fi

TOOLCHAIN_BUILD_PREFIX=$BUILD_OUT/prefix

ARCH=$HOST_ARCH

# Disable futimens@GLIBC_2.6 not available in system on server with very old libc.so
CFLAGS_FOR_BUILD="-O2 -I$TOOLCHAIN_BUILD_PREFIX/include -DDISABLE_FUTIMENS"
LDFLAGS_FOR_BUILD="-L$TOOLCHAIN_BUILD_PREFIX/lib"

# Statically link stdc++ to eliminate dependency on outdated libctdc++.so in old 32-bit
# linux system, and libgcc_s_sjlj-1.dll and libstdc++-6.dll on windows
LLVM_VERSION="`echo $TOOLCHAIN | tr '-' '\n' | tail -n 1`"
if [ "$MINGW" = "yes" -o "$HOST_TAG" = "linux-x86" -o "$LLVM_VERSION" \> "3.4" ]; then
    LDFLAGS_FOR_BUILD=$LDFLAGS_FOR_BUILD" -static-libstdc++"
    if [ "$CC" = "${CC%%clang*}" ]; then
        LDFLAGS_FOR_BUILD=$LDFLAGS_FOR_BUILD" -static-libgcc"
    fi
fi

CFLAGS="$CFLAGS $CFLAGS_FOR_BUILD $HOST_CFLAGS"
CXXFLAGS="$CXXFLAGS $CFLAGS_FOR_BUILD $HOST_CFLAGS"  # polly doesn't look at CFLAGS !
LDFLAGS="$LDFLAGS $LDFLAGS_FOR_BUILD $HOST_LDFLAGS"
export CC CXX CFLAGS CXXFLAGS LDFLAGS CFLAGS_FOR_BUILD LDFLAGS_FOR_BUILD REQUIRES_RTTI=1 ARCH

if [ "$DARWIN" = "yes" ]; then
    # To stop /usr/bin/install -s calls strip on darwin binary
    export KEEP_SYMBOLS=1
    # Disable polly for now
    POLLY=no
fi

if [ "$POLLY" = "yes" -a ! -d "$SRC_DIR/$TOOLCHAIN/polly" ] ; then
    dump "Disable polly because $SRC_DIR/$TOOLCHAIN/polly doesn't exist"
    POLLY=no
fi

EXTRA_CONFIG_FLAGS=
rm -rf $SRC_DIR/$TOOLCHAIN/llvm/tools/polly
if [ "$POLLY" = "yes" ]; then
    # crate symbolic link
    ln -s ../../polly $SRC_DIR/$TOOLCHAIN/llvm/tools

    # build polly dependencies
    unpack_archive "$GMP_SOURCE" "$BUILD_OUT"
    fail_panic "Couldn't unpack $SRC_DIR/gmp/gmp-$GMP_VERSION to $BUILD_OUT"

    GMP_BUILD_OUT=$BUILD_OUT/gmp-$GMP_VERSION
    cd $GMP_BUILD_OUT
    fail_panic "Couldn't cd into gmp build path: $GMP_BUILD_OUT"

    OLD_ABI="${ABI}"
    export ABI=$HOST_GMP_ABI  # needed to build 32-bit on 64-bit host
    run ./configure \
        --prefix=$TOOLCHAIN_BUILD_PREFIX \
        --host=$ABI_CONFIGURE_HOST \
        --build=$ABI_CONFIGURE_BUILD \
        --disable-shared \
        --disable-nls \
        --enable-cxx
    fail_panic "Couldn't configure gmp"
    run make -j$NUM_JOBS
    fail_panic "Couldn't compile gmp"
    run make install
    fail_panic "Couldn't install gmp to $TOOLCHAIN_BUILD_PREFIX"
    ABI="$OLD_ABI"

    CLOOG_BUILD_OUT=$BUILD_OUT/cloog
    mkdir -p $CLOOG_BUILD_OUT && cd $CLOOG_BUILD_OUT
    fail_panic "Couldn't create cloog build path: $CLOOG_BUILD_OUT"

    run $SRC_DIR/$TOOLCHAIN/llvm/tools/polly/utils/cloog_src/configure \
        --prefix=$TOOLCHAIN_BUILD_PREFIX \
        --host=$ABI_CONFIGURE_HOST \
        --build=$ABI_CONFIGURE_BUILD \
        --with-gmp-prefix=$TOOLCHAIN_BUILD_PREFIX \
        --disable-shared \
        --disable-nls
    fail_panic "Couldn't configure cloog"
    run make -j$NUM_JOBS
    fail_panic "Couldn't compile cloog"
    run make install
    fail_panic "Couldn't install cloog to $TOOLCHAIN_BUILD_PREFIX"

    EXTRA_CONFIG_FLAGS="--with-cloog=$TOOLCHAIN_BUILD_PREFIX --with-isl=$TOOLCHAIN_BUILD_PREFIX"

    # Allow text relocs when linking LLVMPolly.dylib against statically linked libgmp.a
    if [ "$HOST_TAG32" = "darwin-x86" -o "$DARWIN" = "yes" ]; then   # -a "$HOST_ARCH" = "x86"
        LDFLAGS="$LDFLAGS -read_only_relocs suppress"
        export LDFLAGS
    fi
fi # POLLY = yes

# configure the toolchain
dump "Configure: $TOOLCHAIN toolchain build"
LLVM_BUILD_OUT=$BUILD_OUT/llvm
mkdir -p $LLVM_BUILD_OUT && cd $LLVM_BUILD_OUT
fail_panic "Couldn't cd into llvm build path: $LLVM_BUILD_OUT"

# Only start using integrated bc2native source >= 3.3 by default
LLVM_VERSION_MAJOR=`echo $LLVM_VERSION | tr '.' '\n' | head -n 1`
LLVM_VERSION_MINOR=`echo $LLVM_VERSION | tr '.' '\n' | tail -n 1`
if [ $LLVM_VERSION_MAJOR -lt 3 ]; then
    USE_PYTHON=yes
elif [ $LLVM_VERSION_MAJOR -eq 3 ] && [ $LLVM_VERSION_MINOR -lt 3 ]; then
    USE_PYTHON=yes
fi

if [ "$USE_PYTHON" != "yes" ]; then
    # Refresh intermediate source
    rm -f $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native/*.cpp
    rm -f $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native/*.c
    rm -f $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native/*.h
    run cp -a $NDK_DIR/tests/abcc/jni/*.cpp $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native
    run cp -a $NDK_DIR/tests/abcc/jni/Abcc.h $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native
    run cp -a $NDK_DIR/tests/abcc/jni/host/*.cpp $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native
    run cp -a $NDK_DIR/tests/abcc/jni/host/*.h $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native
    run cp -a $NDK_DIR/tests/abcc/jni/llvm_${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR}.h $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native/llvm_version.h
    run cp -a $NDK_DIR/tests/abcc/jni/mman-win32/mman.[ch] $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native
    export LLVM_TOOLS_FILTER="PARALLEL_DIRS:=\$\$(PARALLEL_DIRS:%=% ndk-bc2native)"
fi

BINUTILS_VERSION=$(get_default_binutils_version_for_llvm $TOOLCHAIN)

run $SRC_DIR/$TOOLCHAIN/llvm/configure \
    --prefix=$TOOLCHAIN_BUILD_PREFIX \
    --host=$ABI_CONFIGURE_HOST \
    --build=$ABI_CONFIGURE_BUILD \
    --with-bug-report-url=$DEFAULT_ISSUE_TRACKER_URL \
    --enable-targets=arm,mips,x86,aarch64 \
    --enable-optimized \
    --with-binutils-include=$SRC_DIR/binutils/binutils-$BINUTILS_VERSION/include \
    $EXTRA_CONFIG_FLAGS
fail_panic "Couldn't configure llvm toolchain"

# build llvm/clang
dump "Building : llvm toolchain [this can take a long time]."
cd $LLVM_BUILD_OUT
run make -j$NUM_JOBS $MAKE_FLAGS
fail_panic "Couldn't compile llvm toolchain"

if [ "$CHECK" = "yes" -a "$MINGW" != "yes" -a "$DARWIN" != "yes" ] ; then
    # run the regression test
    dump "Running  : llvm toolchain regression test"
    cd $LLVM_BUILD_OUT
    run make check-all
    fail_warning "Couldn't pass all llvm regression test"  # change to fail_panic later
    if [ "$POLLY" = "yes" ]; then
        dump "Running  : polly toolchain regression test"
        cd $LLVM_BUILD_OUT
        run make polly-test -C tools/polly/test
        fail_warning "Couldn't pass all polly regression test"  # change to fail_panic later
    fi
fi

# install the toolchain to its final location
dump "Install  : llvm toolchain binaries"
cd $LLVM_BUILD_OUT && run make install $MAKE_FLAGS
fail_panic "Couldn't install llvm toolchain to $TOOLCHAIN_BUILD_PREFIX"

# copy arm_neon_x86.h from GCC
GCC_SRC_DIR=$SRC_DIR/gcc/gcc-$DEFAULT_GCC32_VERSION
cp -a $GCC_SRC_DIR/gcc/config/i386/arm_neon.h $TOOLCHAIN_BUILD_PREFIX/lib/clang/$LLVM_VERSION/include/arm_neon_x86.h

# Since r156448, llvm installs a separate llvm-config-host when cross-compiling. Use llvm-config-host if this
# exists otherwise llvm-config.
# Note, llvm-config-host should've really been called llvm-config-build and the following changes fix this by
# doing this rename and also making a proper llvm-config-host;
# https://android-review.googlesource.com/#/c/64261/
# https://android-review.googlesource.com/#/c/64263/
# .. with these fixes in place, Darwin mclinker can be cross-compiled and Wine is not needed for Windows cross
# To my mind, llvm-config-host is a misnomer and it should be llvm-config-build.
LLVM_CONFIG=$TOOLCHAIN_BUILD_PREFIX/bin/llvm-config
if [ -f $TOOLCHAIN_BUILD_PREFIX/bin/llvm-config-host ] ; then
    LLVM_CONFIG=$TOOLCHAIN_BUILD_PREFIX/bin/llvm-config-host
fi

# build mclinker only against default the LLVM version, once
if [ "$MCLINKER" = "yes" -o "$TOOLCHAIN" = "llvm-$DEFAULT_LLVM_VERSION" ] ; then
    dump "Copy     : mclinker source"
    MCLINKER_SRC_DIR=$BUILD_OUT/mclinker
    mkdir -p $MCLINKER_SRC_DIR
    fail_panic "Couldn't create mclinker source directory: $MCLINKER_SRC_DIR"

    run copy_directory "$SRC_DIR/mclinker" "$MCLINKER_SRC_DIR"
    fail_panic "Couldn't copy mclinker source: $MCLINKER_SRC_DIR"

    CXXFLAGS="$CXXFLAGS -fexceptions"  # optimized/ScriptParser.cc needs it
    if [ "$MINGW" = "yes" ] ; then
      # Windows dll targets is already build with position independent code, so adding
      # -fPIC is considered insult to mingw-64 compilers which complains and dies
      # if -Werror is also on
      #
      #  addng .../mclinker/lib/ADT/StringEntry.cpp:1:0: error: -fPIC ignored for target (all code is position independent) [-Werror]
      #
        CXXFLAGS="$CXXFLAGS -Wno-error"
    fi
    export CXXFLAGS

    cd $MCLINKER_SRC_DIR && run ./autogen.sh
    fail_panic "Couldn't run autogen.sh in $MCLINKER_SRC_DIR"

    dump "Configure: mclinker against $TOOLCHAIN"
    MCLINKER_BUILD_OUT=$MCLINKER_SRC_DIR/build
    mkdir -p $MCLINKER_BUILD_OUT && cd $MCLINKER_BUILD_OUT
    fail_panic "Couldn't cd into mclinker build path: $MCLINKER_BUILD_OUT"

    run $MCLINKER_SRC_DIR/configure \
        --prefix=$TOOLCHAIN_BUILD_PREFIX \
        --with-llvm-config=$LLVM_CONFIG \
        --host=$ABI_CONFIGURE_HOST \
        --build=$ABI_CONFIGURE_BUILD
    fail_panic "Couldn't configure mclinker"

    dump "Building : mclinker"
    if [ "$MINGW" = "yes" ]; then
        MAKE_FLAGS="$MAKE_FLAGS LIBS=-lshlwapi" # lib/Object/SectionMap.cpp needs PathMatchSpec to replace fnmatch()
    fi
    cd $MCLINKER_BUILD_OUT
    run make -j$NUM_JOBS $MAKE_FLAGS CXXFLAGS="$CXXFLAGS"
    fail_panic "Couldn't compile mclinker"

    dump "Install  : mclinker"
    cd $MCLINKER_BUILD_OUT && run make install $MAKE_FLAGS
    fail_panic "Couldn't install mclinker to $TOOLCHAIN_BUILD_PREFIX"

    if [ "$CHECK" = "yes" -a "$MINGW" != "yes" -a "$DARWIN" != "yes" ] ; then
        # run the regression test
        dump "Running  : mclinker regression test"
        cd $MCLINKER_BUILD_OUT
        run make check
        fail_warning "Couldn't pass all mclinker regression test"  # change to fail_panic later
    fi
fi

# remove redundant bits
rm -rf $TOOLCHAIN_BUILD_PREFIX/docs
rm -rf $TOOLCHAIN_BUILD_PREFIX/include
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/*.a
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/*.la
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/pkgconfig
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/lib[cp]*.so
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/lib[cp]*.dylib
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/B*.so
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/B*.dylib
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/LLVMH*.so
rm -rf $TOOLCHAIN_BUILD_PREFIX/lib/LLVMH*.dylib
if [ -f $TOOLCHAIN_BUILD_PREFIX/bin/ld.lite${HOST_EXE} ]; then
    # rename ld.lite to ld.mcld
    rm -rf $TOOLCHAIN_BUILD_PREFIX/bin/ld.[bm]*
    mv -f $TOOLCHAIN_BUILD_PREFIX/bin/ld.lite${HOST_EXE} $TOOLCHAIN_BUILD_PREFIX/bin/ld.mcld${HOST_EXE}
else
    rm -rf $TOOLCHAIN_BUILD_PREFIX/bin/ld.bcc
fi
rm -rf $TOOLCHAIN_BUILD_PREFIX/share

UNUSED_LLVM_EXECUTABLES="
bugpoint c-index-test clang-check clang-format clang-tblgen lli llvm-bcanalyzer
llvm-config llvm-config-host llvm-cov llvm-diff llvm-dwarfdump llvm-extract llvm-ld
llvm-mc llvm-nm llvm-mcmarkup llvm-objdump llvm-prof llvm-ranlib llvm-readobj llvm-rtdyld
llvm-size llvm-stress llvm-stub llvm-symbolizer llvm-tblgen llvm-vtabledump macho-dump cloog
llvm-vtabledump lli-child-target not count FileCheck llvm-profdata"

for i in $UNUSED_LLVM_EXECUTABLES; do
    rm -f $TOOLCHAIN_BUILD_PREFIX/bin/$i
    rm -f $TOOLCHAIN_BUILD_PREFIX/bin/$i.exe
done

test -z "$STRIP" && STRIP=strip
find $TOOLCHAIN_BUILD_PREFIX/bin -maxdepth 1 -type f -exec $STRIP {} \;
# Note that MacOSX strip generate the follow error on .dylib:
# "symbols referenced by indirect symbol table entries that can't be stripped "
find $TOOLCHAIN_BUILD_PREFIX/lib -maxdepth 1 -type f \( -name "*.dll" -o -name "*.so" \) -exec $STRIP {} \;

# For now, le64-tools is just like le32 ones
if [ -f "$TOOLCHAIN_BUILD_PREFIX/bin/ndk-link${HOST_EXE}" ]; then
    run ln -s ndk-link${HOST_EXE} $TOOLCHAIN_BUILD_PREFIX/bin/le32-none-ndk-link${HOST_EXE}
    run ln -s ndk-link${HOST_EXE} $TOOLCHAIN_BUILD_PREFIX/bin/le64-none-ndk-link${HOST_EXE}
fi
if [ -f "$TOOLCHAIN_BUILD_PREFIX/bin/ndk-strip${HOST_EXE}" ]; then
    run ln -s ndk-strip${HOST_EXE} $TOOLCHAIN_BUILD_PREFIX/bin/le32-none-ndk-strip${HOST_EXE}
    run ln -s ndk-strip${HOST_EXE} $TOOLCHAIN_BUILD_PREFIX/bin/le64-none-ndk-strip${HOST_EXE}
fi
if [ -f "$TOOLCHAIN_BUILD_PREFIX/bin/ndk-translate${HOST_EXE}" ]; then
    run ln -s ndk-translate${HOST_EXE} $TOOLCHAIN_BUILD_PREFIX/bin/le32-none-ndk-translate${HOST_EXE}
    run ln -s ndk-translate${HOST_EXE} $TOOLCHAIN_BUILD_PREFIX/bin/le64-none-ndk-translate${HOST_EXE}
fi

# install script
if [ "$USE_PYTHON" != "yes" ]; then
    # Remove those intermediate cpp
    rm -f $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native/*.cpp
    rm -f $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native/*.c
    rm -f $SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native/*.h
else
    cp -p "$SRC_DIR/$TOOLCHAIN/llvm/tools/ndk-bc2native/ndk-bc2native.py" "$TOOLCHAIN_BUILD_PREFIX/bin/"
fi

# copy to toolchain path
run copy_directory "$TOOLCHAIN_BUILD_PREFIX" "$TOOLCHAIN_PATH"

# create analyzer/++ scripts
ABIS=$PREBUILT_ABIS
# temp hack before 64-bit ABIs are part of PREBUILT_ABIS
if [ "$ABIS" != "${ABIS%%64*}" ]; then
    ABIS="$PREBUILT_ABIS arm64-v8a x86_64 mips64"
fi
ABIS=$ABIS
for ABI in $ABIS; do
    ANALYZER_PATH="$TOOLCHAIN_PATH/bin/$ABI"
    ANALYZER="$ANALYZER_PATH/analyzer"
    mkdir -p "$ANALYZER_PATH"
    case "$ABI" in
      armeabi)
          LLVM_TARGET=armv5te-none-linux-androideabi
          ;;
      armeabi-v7a|armeabi-v7a-hard)
          LLVM_TARGET=armv7-none-linux-androideabi
          ;;
      arm64-v8a)
          LLVM_TARGET=aarch64-none-linux-android
          ;;
      x86)
          LLVM_TARGET=i686-none-linux-android
          ;;
      x86_64)
          LLVM_TARGET=x86_64-none-linux-android
          ;;
      mips|mips32r6)
          LLVM_TARGET=mipsel-none-linux-android
          ;;
      mips64)
          LLVM_TARGET=mips64el-none-linux-android
          ;;
      *)
        dump "ERROR: Unsupported NDK ABI: $ABI"
        exit 1
    esac

    cat > "${ANALYZER}" <<EOF
if [ "\$1" != "-cc1" ]; then
    \`dirname \$0\`/../clang -target $LLVM_TARGET "\$@"
else
    # target/triple already spelled out.
    \`dirname \$0\`/../clang "\$@"
fi
EOF
    cat > "${ANALYZER}++" <<EOF
if [ "\$1" != "-cc1" ]; then
    \`dirname \$0\`/../clang++ -target $LLVM_TARGET "\$@"
else
    # target/triple already spelled out.
    \`dirname \$0\`/../clang++ "\$@"
fi
EOF
    chmod 0755 "${ANALYZER}" "${ANALYZER}++"

    if [ -n "$HOST_EXE" ] ; then
        cat > "${ANALYZER}.cmd" <<EOF
@echo off
if "%1" == "-cc1" goto :L
%~dp0\\..\\clang${HOST_EXE} -target $LLVM_TARGET %*
if ERRORLEVEL 1 exit /b 1
goto :done
:L
rem target/triple already spelled out.
%~dp0\\..\\clang${HOST_EXE} %*
if ERRORLEVEL 1 exit /b 1
:done
EOF
        cat > "${ANALYZER}++.cmd" <<EOF
@echo off
if "%1" == "-cc1" goto :L
%~dp0\\..\\clang++${HOST_EXE} -target $LLVM_TARGET %*
if ERRORLEVEL 1 exit /b 1
goto :done
:L
rem target/triple already spelled out.
%~dp0\\..\\clang++${HOST_EXE} %*
if ERRORLEVEL 1 exit /b 1
:done
EOF
        chmod 0755 "${ANALYZER}.cmd" "${ANALYZER}++.cmd"
    fi
done

# copy SOURCES file if present
if [ -f "$SRC_DIR/SOURCES" ]; then
    cp "$SRC_DIR/SOURCES" "$TOOLCHAIN_PATH/SOURCES"
fi

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE="$TOOLCHAIN-$HOST_TAG.tar.bz2"
    SUBDIR=$(get_toolchain_install_subdir $TOOLCHAIN $HOST_TAG)
    dump "Packaging $ARCHIVE"
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$SUBDIR"
fi

dump "Done."
if [ -z "$OPTION_BUILD_OUT" ] ; then
    rm -rf $BUILD_OUT
fi
