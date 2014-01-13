# Check if some platform headers can be included alone
# See b.android.com/64679 for one of them
#

export ANDROID_NDK_ROOT=$NDK

NDK_BUILDTOOLS_PATH=$NDK/build/tools
. $NDK/build/tools/prebuilt-common.sh

INTERNAL_HEADERS="sys/_errdefs.h sys/_sigdefs.h sys/_system_properties.h"

for API_LEVEL in $API_LEVELS; do
    for ARCH in $DEFAULT_ARCHS; do
        if [ -d $ANDROID_NDK_ROOT/platforms/android-$API_LEVEL/arch-$ARCH ]; then
            HEADERS=`cd $ANDROID_NDK_ROOT/platforms/android-$API_LEVEL/arch-$ARCH/usr/include ; ls *.h sys/*.h android/*.h EGL/*.h GLES/*.h GLES2/*.h GLES3/*.h OMXAL/*.h SLES/*.h 2> /dev/null`
            #echo $API_LEVEL $ARCH HEADERS=$HEADERS
            ABIS=$(commas_to_spaces $(convert_arch_to_abi $ARCH))
            for ABI in $ABIS; do
                for HEADER in $HEADERS; do
                    if [ "$INTERNAL_HEADERS" = "${INTERNAL_HEADERS%%$HEADER*}" ] ; then
                        #echo Compiling with $HEADER
                        $ANDROID_NDK_ROOT/ndk-build -B APP_CFLAGS=-DHEADER=\"\<$HEADER\>\" APP_PLATFORM=android-$API_LEVEL APP_ABI=$ABI 1>/dev/null 2>&1
                        fail_panic "Can't compile header $ANDROID_NDK_ROOT/platforms/android-$API_LEVEL/arch-$ARCH/usr/include/$HEADER alone.
To reproduce: $ANDROID_NDK_ROOT/ndk-build -B APP_CFLAGS=-DHEADER=\"\<$HEADER\>\" APP_PLATFORM=android-$API_LEVEL APP_ABI=$ABI"
                    fi
                done
            done
        fi
    done
done
