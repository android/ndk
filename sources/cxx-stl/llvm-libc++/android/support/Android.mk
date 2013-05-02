LOCAL_PATH := $(call my-dir)

android_support_c_includes := $(LOCAL_PATH)/include

android_support_sources := \
    src/locale_support.c \
    src/nl_types_support.c \
    src/stdlib_support.c \
    src/wchar_support.c \
    src/wctype_support.c \

include $(CLEAR_VARS)
LOCAL_MODULE := llvm_libc++_support_android
LOCAL_SRC_FILES := $(android_support_sources)
LOCAL_C_INCLUDES := $(android_support_c_includes)
LOCAL_EXPORT_C_INCLUDES := $(android_support_c_includes)
include $(BUILD_STATIC_LIBRARY)
