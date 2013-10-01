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
# Build a abcc package. This exploits build-on-device-toolchain.sh and
# needs SDK help.
#

PROGDIR=`cd $(dirname $0) && pwd`
NDK_BUILDTOOLS_PATH=$PROGDIR/../../build/tools
. $NDK_BUILDTOOLS_PATH/prebuilt-common.sh

SDK_DIR=
register_var_option "--sdk-dir=<path>" SDK_DIR "SDK installation directory (Required)"

SDK_TARGET=
register_var_option "--sdk-target=<str>" SDK_TARGET "SDK target for building APK (Use 'android list target' to check)"

NDK_DIR=$ANDROID_NDK_ROOT
register_var_option "--ndk-dir=<path>" NDK_DIR "NDK installation directory"

ABCC_DIR=$PROGDIR
register_var_option "--abcc-dir=<path>" ABCC_DIR "Compiler app directory"

BUILD_DIR=/tmp/ndk-$USER/build
register_var_option "--build-dir=<path>" BUILD_DIR "Specify temporary build dir"

OUT_DIR=/tmp/ndk-$USER/out
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory directly"

ABIS=
register_var_option "--abis=<target>" ABIS "List which targets you use (comma for split)"

DEFAULT_TMP_SRC_DIR=/tmp/ndk-$USER/ndk-toolchain-source-`date +%s`
SRC_DIR=$DEFAULT_TMP_SRC_DIR
register_var_option "--src-dir=<path>" SRC_DIR "Specify an existing toolchain source"

ONLY_ASSETS=
do_only_assets_option () { ONLY_ASSETS=yes; }
register_option "--only-assets" do_only_assets_option "Build toolchain only under prebuilts/assets/ instead of whole app"

DEBUG=
do_debug_option () { DEBUG=yes; }
register_option "--no-share-system-uid" do_debug_option "Just for testing. Be careful of device directory permission issue!"

TESTING=
do_testing_option () { TESTING=yes; }
register_option "--testing" do_testing_option "Package all prebuilts into abcc for testing"

NO_REBUILD_ASSETS=
do_no_rebuild_assets_option () { NO_REBUILD_ASSETS=yes; }
register_option "--no-rebuild-assets" do_no_rebuild_assets_option "Use existing toolchain prebuilt assets instead of rebuilding them"

register_jobs_option

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"This script can be used to build abcc, which contains all toolchain
we need for on-device compilation. This script also needs SDK with binaries,
like ant, aapt, android, ...etc, since they are necessary to produce APK."

extract_parameters "$@"
ABIS=$(commas_to_spaces $ABIS)
test -z "$ABIS" && ABIS="$PREBUILT_ABIS"
BUILDTOOLS=$NDK_DIR/build/tools
ABCC_PREBUILT_ASSETS=$ABCC_DIR/prebuilts/assets
ABCC=`basename $ABCC_DIR`
FLAGS=
test "$VERBOSE" = "yes" && FLAGS=$FLAGS" --verbose"
test "$VERBOSE2" = "yes" && FLAGS=$FLAGS" --verbose"
FLAGS="$FLAGS -j$NUM_JOBS"
test "$TESTING" = "yes" && FLAGS=$FLAGS" --testing"

#
# First: Build toolchain assets
#

if [ "$NO_REBUILD_ASSETS" = "yes" ]; then
  test -z "`ls $ABCC_PREBUILT_ASSETS 2> /dev/null`" && dump "[WARNING] No toolchain assets found!"
else
  test "$SRC_DIR" != "$DEFAULT_TMP_SRC_DIR" && check_toolchain_src_dir "$SRC_DIR"
  test "$SRC_DIR" = "$DEFAULT_TMP_SRC_DIR" && run $BUILDTOOLS/download-toolchain-sources.sh $SRC_DIR
  run rm -rf $ABCC_PREBUILT_ASSETS/*
  for ABI in $ABIS; do
    run $BUILDTOOLS/build-on-device-toolchain.sh --ndk-dir=$NDK_DIR --build-dir=$BUILD_DIR --out-dir=$ABCC_PREBUILT_ASSETS/$ABI --abi=$ABI --no-sync $FLAGS $SRC_DIR
    fail_panic "Could not build device toolchain."
  done
fi

test "$ONLY_ASSETS" = "yes" && exit

#
# Second: Check SDK
#

test -z "$SDK_DIR" && dump "--sdk-dir is required." && exit 1
test ! -f "$SDK_DIR/tools/android" && dump "--sdk-dir is not a valid SDK." && exit 1
test `$SDK_DIR/tools/android list target | grep '^id' | wc -l` -eq 0 && "Please download at least one target first." && exit 1

# Ask users for SDK configuration
if [ `$SDK_DIR/tools/android list target | grep '^id' | wc -l` -ne 1 ] && [ -z "$SDK_TARGET" ]; then
  DEFAULT_TARGET="`$SDK_DIR/tools/android list target | grep '^id' | head -n 1 | awk '{print $4}'`"
  echo "* Which target do you want? [$DEFAULT_TARGET]"
  for line in "`$SDK_DIR/tools/android list target | grep '^id'`"; do
    echo "-- `echo $line | awk '{print $4}'`"
  done
  echo ""
  read SDK_TARGET
  test -z "$SDK_TARGET" && SDK_TARGET=$DEFAULT_TARGET
elif [ -z "$SDK_TARGET" ]; then
  SDK_TARGET=`$SDK_DIR/tools/android list target | grep '^id' | awk '{print $4}'`
fi
dump "SDK target: $SDK_TARGET"

#
# Third: Build apk
#

run rm -rf $BUILD_DIR
run mkdir -p $BUILD_DIR $OUT_DIR

run cd $BUILD_DIR
run cp -a $ABCC_DIR $ABCC

run cd $BUILD_DIR/$ABCC
run $SDK_DIR/tools/android update project -p . -t "$SDK_TARGET"
if [ $? -ne 0 ]; then
  dump "Cannot create build.xml. Abort."
  exit 1
fi

for ABI in $ABIS; do
  run rm -rf obj libs
  run $NDK_DIR/ndk-build -B APP_ABI=$ABI -C jni
  fail_panic "Build ndk-build failed. Abort."
  if [ "$DEBUG" = "yes" ]; then
    run rm -f AndroidManifest.xml
    run cp -a AndroidManifest.xml.debug AndroidManifest.xml
    run ant debug -Dasset.dir=prebuilts/assets/$ABI
    fail_panic "Build dex failed. Abort."
    run cp -a bin/$ABCC-debug.apk $OUT_DIR/$ABCC-$ABI.apk
  else  # DEBUG != yes
    run ant release -Dasset.dir=prebuilts/assets/$ABI
    fail_panic "Build dex failed. Abort."
    run cp -a bin/$ABCC-release-unsigned.apk $OUT_DIR/$ABCC-$ABI-unsigned.apk
  fi
done

run cd $OUT_DIR
run rm -rf $BUILD_DIR

dump "Done. Compiler app is under $OUT_DIR"
test "$DEBUG" != "yes" && dump "[WARNING] APK has not been signed nor aligned!"
exit 0
