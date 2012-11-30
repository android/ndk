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

ABIS=
for OPT; do
  case $OPT in
    APP_ABI=*)
      ABIS=${OPT##APP_ABI=}
      ;;
  esac
done

if [ -z "$ABIS" ]; then
  ABIS="armeabi armeabi-v7a x86 mips"
fi

# Step 1: Build prebuilt libraries.
$NDK/ndk-build -C "$PREBUILTS_DIR"
if [ $? != 0 ]; then
  echo "ERROR: Can't build prebuilt libraries!"
  exit 1
fi

# Step 2: Build the project
PREBUILTS_DIR=$PREBUILTS_DIR $NDK/ndk-build -C "$PROGDIR"
if [ $? != 0 ]; then
  echo "ERROR: Can't build project!"
  exit 1
fi

# Step 3: Check that the prebuilt shared library was copied to the
#         right location.
#

FAILURES=0
for ABI in $ABIS; do
  SHARED_LIB=$OUT/$ABI/libfoo.so
  STATIC_LIB=$OUT/$ABI/libbar.a
  printf "Checking for $ABI shared library: "
  if [ ! -f "$SHARED_LIB" ]; then
    printf "KO! missing file: $SHARED_LIB\n"
    FAILURES=$(( $FAILURES + 1 ))
  else
    printf "ok\n"
  fi

  printf "Checking for $ABI static library: "
  if [ -f "$STATIC_LIB" ]; then
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
