# Check if ARM g++ no longer gives pointless warning about the mangling of <va_list> has changed in GCC 4.4
# See https://android-review.googlesource.com/#/c/42274/
#

NDK_BUILDTOOLS_PATH=$NDK/build/tools
. $NDK/build/tools/prebuilt-common.sh

SYSTEM=$(get_prebuilt_host_tag)

ARM_GPP=$NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/$SYSTEM/bin/arm-linux-androideabi-g++

OUT=$(echo "#include <stdarg.h>
void foo(va_list v) { }" | $ARM_GPP -x c++ -c -o /dev/null - 2>&1)

if [ -z "$OUT" ]; then
  echo "ARM g++ no longer gives pointless warning about the mangling of <va_list> has changed in GCC 4.4"
  exit 0
else
  echo "ERROR: ARM g++ still gives pointless warning about the mangling of <va_list> has changed in GCC 4.4"
  exit 1
fi
