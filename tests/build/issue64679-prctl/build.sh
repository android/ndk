# Check if some platform headers can be included alone
# See b.android.com/64679 for one of them
#

export ANDROID_NDK_ROOT=$NDK

NDK_BUILDTOOLS_PATH=$NDK/build/tools
. $NDK/build/tools/prebuilt-common.sh

JOBS=1
for OPT; do
    case $OPT in
        -j*)
            JOBS=${OPT##-j}
            ;;
        --jobs=*)
            JOBS=${OPT##--jobs=}
            ;;
    esac
done

INTERNAL_HEADERS="sys/_errdefs.h sys/_sigdefs.h sys/_system_properties.h"
INVALID_HEADERS_FOR_64BIT="time64.h sys/user.h"  # ToDo: remove sys/user.h later once __u64 and __u32 are defined for x86_64

# This header doesn't compile unless included from C++ with
# --std=c++-0x or higher, so skip it here.
INTERNAL_HEADERS="$INTERNAL_HEADERS uchar.h"

for API_LEVEL in $API_LEVELS; do
    for ARCH in $DEFAULT_ARCHS; do
        if [ ! -d $ANDROID_NDK_ROOT/platforms/android-$API_LEVEL/arch-$ARCH ]; then
            continue
        fi
        HEADERS=$(cd $ANDROID_NDK_ROOT/platforms/android-$API_LEVEL/arch-$ARCH/usr/include ; ls *.h sys/*.h android/*.h EGL/*.h GLES/*.h GLES2/*.h GLES3/*.h OMXAL/*.h SLES/*.h 2> /dev/null)
        #echo $API_LEVEL $ARCH HEADERS=$HEADERS
        ABIS=$(commas_to_spaces $(convert_arch_to_abi $ARCH))
        for ABI in $ABIS; do
            # Create temporary project
            PROJECT_DIR=${TMPDIR:-/tmp}/$USER-ndk-test-system-headers/android-$API_LEVEL-$ABI
            (mkdir -p "$PROJECT_DIR" && rm -rf "$PROJECT_DIR"/*) || panic "Can't create dir: $PROJECT_DIR"
            mkdir "$PROJECT_DIR"/jni
            cat > $PROJECT_DIR/jni/Application.mk <<EOF
# Auto-generated - DO NOT EDIT
APP_PLATFORM=android-$API_LEVEL
APP_ABI=$ABI
APP_MODULES=libfoo
EOF
            ANDROID_MK=$PROJECT_DIR/jni/Android.mk
            cat > $ANDROID_MK <<EOF
# Auto-generated - DO NOT EDIT
LOCAL_PATH := \$(call my-dir)

include \$(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := \\
EOF
            for HEADER in $HEADERS; do
                if [ "$INTERNAL_HEADERS" != "${INTERNAL_HEADERS%%$HEADER*}" ] ; then
                    continue;
                fi
                if [ "$ABI" != "${ABI%%64*}" ] ; then
                    if [ "$INVALID_HEADERS_FOR_64BIT" != "${INVALID_HEADERS_FOR_64BIT%%$HEADER*}" ] ; then
                        continue;
                    fi
                fi
                NAME=$(echo "$HEADER" | tr '/' '__' | tr '.' '_' | tr '-' '_')
                SRC=$NAME.c
                SRC_FILE=$PROJECT_DIR/jni/$SRC
                cat > $PROJECT_DIR/jni/$SRC <<EOF
/* Auto-generated - DO NOT EDIT */
#include <$HEADER>

char dummy_$NAME = 0;
EOF
                echo "    $SRC \\" >> $ANDROID_MK
            done
            cat >> $ANDROID_MK <<EOF

include \$(BUILD_STATIC_LIBRARY)

EOF
            echo "Checking headers for android-$API_LEVEL/$ABI"
            $ANDROID_NDK_ROOT/ndk-build -C "$PROJECT_DIR" -B -j$JOBS 1>/dev/null 2>&1
            fail_panic "Can't compile header for android-$API_LEVEL/$ABI, to reproduce: $ANDROID_NDK_ROOT/ndk-build -C $PROJECT_DIR"

            rm -rf "$PROJECT_DIR"
        done  # for ABI
    done  # for ARCH
done  # for API_LEVEL
