# Check that the libc.so for all platforms, and all architectures
# Does not export 'atexit' and '__dso_handle' symbols.
#
export ANDROID_NDK_ROOT=$NDK

NDK_BUILDTOOLS_PATH=$NDK/build/tools
. $NDK/build/tools/prebuilt-common.sh
echo DEFAULT_ARCHS=$DEFAULT_ARCHS

LIBRARIES=
for ARCH in $DEFAULT_ARCHS; do
  LIB=$(cd $NDK && find platforms -name "libc.so" | sed -e 's!^!'$NDK'/!' | grep arch-$ARCH)
  LIBRARIES=$LIBRARIES" $LIB"
done

FAILURE=
COUNT=0
for LIB in $LIBRARIES; do
  COUNT=$(( $COUNT + 1 ))
  echo "Checking: $LIB"
  readelf -s $LIB | grep -q -F " atexit"
  if [ $? = 0 ]; then
    echo "ERROR: $NDK/$LIB exposes 'atexit'!" >&2
    FAILURE=true
  fi
  readelf -s $LIB | grep -q -F " __dso_handle"
  if [ $? = 0 ]; then
    echo "ERROR: $NDK/$LIB exposes '__dso_handle'!" >&2
    FAILURE=true
  fi
done

if [ "$COUNT" = 0 ]; then
  echo "ERROR: Did not find any libc.so in $NDK/platforms!"
  exit 1
fi

if [ "$FAILURE" ]; then
  exit 1
else
  echo "All $COUNT libc.so are ok!"
  exit 0
fi
