LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := cpufeatures
LOCAL_SRC_FILES := cpu-features.c

ifdef HISTORICAL_NDK_VERSIONS_ROOT
# This is included by the platform build system.
LOCAL_SDK_VERSION := 9
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
else
# This is included by the NDK build system.
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
endif

include $(BUILD_STATIC_LIBRARY)
