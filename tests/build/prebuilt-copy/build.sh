#!/bin/sh

# The purpose of this script is the following:
#
# 1/ Build the libraries under prebuilts/jni/
#
# 2/ Build the project under jni/
#
# 3/ Check that the prebuilt shared library was copied to
#    $NDK_OUT/<abi>/objs.
#
# 4/ Check that the prebuilt static library was not copied to
#    the same directory.
#

PROGDIR=$(dirname "$0")

OUT=$PROGDIR/obj/local
PREBUILTS_DIR=$PROGDIR/prebuilts
PREBUILTS_DIR=$(cd "$PREBUILTS_DIR" && pwd)

if [ -n "$APP_ABI" ]; then
  ABIS="$APP_ABI"
else
  ABIS=
  for OPT; do
    case $OPT in
      APP_ABI=*)
        ABIS=${OPT##APP_ABI=}
        APP_ABI=$ABIS
        ;;
    esac
  done

  if [ -z "$ABIS" ]; then
    ABIS="armeabi armeabi-v7a x86 mips"
  fi
fi

# Step 0: Remove obj/ and libs/ to ensure everything is clean
rm -rf obj/ libs/
rm -rf $PREBUILTS_DIR/obj/ $PREBUILTS_DIR/libs/

# Step 1: Build prebuilt libraries.
if [ -z "$APP_ABI" ]; then
  $NDK/ndk-build -C "$PREBUILTS_DIR"
  RET=$?
else
  $NDK/ndk-build -C "$PREBUILTS_DIR" APP_ABI="$APP_ABI"
  RET=$?
fi

if [ $RET != 0 ]; then
  echo "ERROR: Can't build prebuilt libraries!"
  exit 1
fi

# Step 2: Build the project
if [ -z "$APP_ABI" ]; then
  PREBUILTS_DIR=$PREBUILTS_DIR $NDK/ndk-build -C "$PROGDIR"
  RET=$?
else
  PREBUILTS_DIR=$PREBUILTS_DIR $NDK/ndk-build -C "$PROGDIR" APP_ABI="$APP_ABI"
  RET=$?
fi

if [ $RET != 0 ]; then
  echo "ERROR: Can't build project!"
  exit 1
fi

# Step 3: Check that the prebuilt shared library was copied to the
#         right location.
#

FAILURES=0
for ABI in $ABIS; do
  printf "Checking for $ABI shared library: "
  SHARED_LIB=$(ls $OUT/*$ABI/libfoo.so 2>/dev/null)
  if [ $? != 0 ]; then
    printf "KO! missing file: $SHARED_LIB\n"
    FAILURES=$(( $FAILURES + 1 ))
  else
    printf "ok\n"
  fi

  printf "Checking for $ABI static library: "
  STATIC_LIB=$(ls $OUT/*$ABI/libbar.a 2>/dev/null)
  if [ $? = 0 ]; then
    printf "KO! file should not exist: $STATIC_LIB\n"
    FAILURES=$(( $FAILURES + 1 ))
  else
    printf "ok\n"
  fi
done

if [ "$FAILURES" = 0 ]; then
  echo "Everything's ok. Congratulations!"
  exit 0
else
  echo "Found $FAILURES failures! Please fix ndk-build!"
  exit 1
fi
