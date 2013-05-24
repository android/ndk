#!/bin/sh
#
# Copyright (C) 2013 The Android Open Source Project
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
# Build the host version of the perl executable and place it
# at the right location

PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir>"

PROGRAM_DESCRIPTION=\
"Rebuild the host perl used by the Android NDK.

Where <src-dir> is the location of toolchain sources."

register_try64_option
register_canadian_option
register_jobs_option

BUILD_OUT=/tmp/ndk-$USER/build/perl
OPTION_BUILD_OUT=
register_var_option "--build-out=<path>" OPTION_BUILD_OUT "Set temporary build directory"

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "Specify NDK install directory"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Archive to package directory"

CHECK=no
do_check_option () { CHECK=yes; }
register_option "--check" do_check_option "Check Perl"

extract_parameters "$@"

SUBDIR=$(get_prebuilt_install_prefix)
BIN_OUT="$SUBDIR/bin/perl${HOST_EXE}"
LIB_OUT="$SUBDIR/lib/perl5"

fix_option BUILD_OUT "$OPTION_BUILD_OUT" "build directory"

PERL_VERSION=$DEFAULT_PERL_VERSION

set_parameters ()
{
    SRC_DIR="$1"

    # Check source directory
    #
    if [ -z "$SRC_DIR" ] ; then
        echo "ERROR: Missing source directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$SRC_DIR/perl/perl-$PERL_VERSION" ] ; then
        echo "ERROR: Source directory does not contain perl sources: $SRC_DIR/perl/perl-$PERL_VERSION"
        exit 1
    fi

    log "Using source directory: $SRC_DIR"

}

set_parameters $PARAMETERS

prepare_host_build

if [ "$MINGW" != "yes" -a "$DARWIN" != "yes" ] ; then
    dump "Using C compiler: $CC"
    dump "Using C++ compiler: $CXX"
fi

log "Configuring the build"
mkdir -p $BUILD_OUT && rm -rf $BUILD_OUT/*
prepare_canadian_toolchain $BUILD_OUT

run copy_directory "$SRC_DIR/perl/perl-$PERL_VERSION" "$BUILD_OUT"
fail_panic "Could not copy perl source $SRC_DIR/perl/perl-$PERL_VERSION to build directory $BUILD_OUT"

LIBS_SEARCH=`$CC -print-search-dirs | grep libraries | sed ' s/^.*=// ' | sed ' s/:/ /g '`

cd $BUILD_OUT &&
CFLAGS=$HOST_CFLAGS" -O2 -s" &&
run ./Configure \
    -des \
    -Dprefix=$BUILD_OUT/prefix \
    -Dcc="$CC" \
    -Dcc_as_ld \
    -Dccflags="$CFLAGS" \
    -A prepend:libpth="$LIBS_SEARCH"
fail_panic "Failed to configure the perl-$PERL_VERSION build!"

log "Building perl"
run make -j $NUM_JOBS
fail_panic "Failed to build perl-$PERL_VERSION!"

if [ "$CHECK" = "yes" ]; then
    log "Checking perl"
    run make check
    fail_warning "Failed to check perl-$PERL_VERSION!"
fi

log "Installing perl"
run make install
fail_panic "Failed to install perl-$PERL_VERSION!"

log "Copying executable to prebuilt location"
run copy_file_list "$BUILD_OUT/prefix/bin" $(dirname "$NDK_DIR/$BIN_OUT") perl
fail_panic "Could not copy executable to: $NDK_DIR/$BIN_OUT"

log "Copying library to prebuilt location"
run copy_directory "$BUILD_OUT/prefix/lib" "$NDK_DIR/$LIB_OUT"
fail_panic "Could not copy library to: $NDK_DIR/$LIB_OUT"

if [ "$PACKAGE_DIR" ]; then
    ARCHIVE=ndk-perl-$HOST_TAG.tar.bz2
    dump "Packaging: $ARCHIVE"
    mkdir -p "$PACKAGE_DIR" &&
    pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" "$BIN_OUT" "$LIB_OUT"
    fail_panic "Could not package archive: $PACKAGE_DIR/$ARCHIVE"
fi

log "Cleaning up"
if [ -z "$OPTION_BUILD_OUT" ] ; then
   rm -rf $BUILD_OUT
fi

log "Done."

