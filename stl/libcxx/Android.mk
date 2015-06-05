LOCAL_PATH := $(call my-dir)

# TODO(danalbert): The old build scripts passed the following flags:
#   * -ffunction-sections
#   * -fdata-sections
#   * -fuse-cxa-atexit
#
#   armeabi-v7a-hard)
#       EXTRA_CFLAGS="-mhard-float -D_NDK_MATH_NO_SOFTFP=1"
#       EXTRA_CXXFLAGS="-mhard-float -D_NDK_MATH_NO_SOFTFP=1"
#       EXTRA_LDFLAGS="-Wl,--no-warn-mismatch -lm_hard"
#       FLOAT_ABI="hard"
#       ;;
#   arm64-v8a)
#       EXTRA_CFLAGS="-mfix-cortex-a53-835769"
#       EXTRA_CXXFLAGS="-mfix-cortex-a53-835769"
#       ;;
#   x86|x86_64)
#       # ToDo: remove the following once all x86-based device call JNI function with
#       #       stack aligned to 16-byte
#       EXTRA_CFLAGS="-mstackrealign"
#       EXTRA_CXXFLAGS="-mstackrealign"
#       ;;
#   mips32r6)
#       EXTRA_CFLAGS="-mips32r6"
#       EXTRA_CXXFLAGS="-mips32r6"
#       EXTRA_LDFLAGS="-mips32r6"
#       ;;
#
# Investigate which of these we need to pass, if any, and if they should be
# added to the platform build system.

include $(CLEAR_VARS)

LOCAL_MODULE := libc++_ndk
LOCAL_CLANG := true
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := none
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include ndk/android_support/include
LOCAL_SRC_FILES := \
    src/algorithm.cpp \
    src/bind.cpp \
    src/chrono.cpp \
    src/condition_variable.cpp \
    src/debug.cpp \
    src/exception.cpp \
    src/future.cpp \
    src/hash.cpp \
    src/ios.cpp \
    src/iostream.cpp \
    src/locale.cpp \
    src/memory.cpp \
    src/mutex.cpp \
    src/new.cpp \
    src/optional.cpp \
    src/random.cpp \
    src/regex.cpp \
    src/shared_mutex.cpp \
    src/stdexcept.cpp \
    src/string.cpp \
    src/strstream.cpp \
    src/system_error.cpp \
    src/thread.cpp \
    src/typeinfo.cpp \
    src/utility.cpp \
    src/valarray.cpp \
    src/support/android/locale_android.cpp \

LOCAL_CPPFLAGS := -fexceptions -DLIBCXXABI=1 -D__STDC_FORMAT_MACROS
LOCAL_RTTI_FLAG := -frtti
LOCAL_WHOLE_STATIC_LIBRARIES_arm := libc++abi_ndk libandroid_support
LOCAL_WHOLE_STATIC_LIBRARIES_arm64 := libc++abi_ndk libandroid_support
LOCAL_WHOLE_STATIC_LIBRARIES_mips := libgabi++_static
LOCAL_WHOLE_STATIC_LIBRARIES_mips64 := libgabi++_static
LOCAL_WHOLE_STATIC_LIBRARIES_x86 := libgabi++_static
LOCAL_WHOLE_STATIC_LIBRARIES_x86_64 := libgabi++_static

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libc++_ndk
LOCAL_CLANG := true
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := none
LOCAL_LDFLAGS := -Wl,--version-script,$(LOCAL_PATH)/export_symbols.txt
LOCAL_WHOLE_STATIC_LIBRARIES := libc++abi_ndk
LOCAL_WHOLE_STATIC_LIBRARIES_arm := libunwind_ndk
LOCAL_LDLIBS := -ldl

include $(BUILD_SHARED_LIBRARY)
