# Check if ARM g++ no longer gives pointless warning about the mangling of <va_list> has changed in GCC 4.4
# See https://android-review.googlesource.com/#/c/42274/
#

NDK_BUILDTOOLS_PATH=$NDK/build/tools
. $NDK/build/tools/prebuilt-common.sh

VERSION=4.6

if [ -n "$NDK_TOOLCHAIN_VERSION" ];  then
    case "$NDK_TOOLCHAIN_VERSION" in
        4.4.3|4.6)
           VERSION=$NDK_TOOLCHAIN_VERSION
            ;;
        clang*)
           echo "No need to test clang on this issue"
           exit 0
            ;;
        *)
           echo "ERROR: invalid NDK_TOOLCHAIN_VERSION $NDK_TOOLCHAIN_VERSION"
           exit 1
    esac
fi

SYSTEM=$(get_prebuilt_host_tag)
ARM_GPP=$NDK/toolchains/arm-linux-androideabi-$VERSION/prebuilt/$SYSTEM/bin/arm-linux-androideabi-g++

OUT=$(echo "#include <stdarg.h>
void foo(va_list v) { }" | $ARM_GPP -x c++ -c -o /dev/null - 2>&1)

if [ -z "$OUT" ]; then
  echo "ARM g++ no longer gives pointless warning about the mangling of <va_list> has changed in GCC 4.4"
  exit 0
else
  echo "ERROR: ARM g++ still gives pointless warning about the mangling of <va_list> has changed in GCC 4.4"
  exit 1
fi
