LOCAL_PATH := $(call my-dir)

ifdef HISTORICAL_NDK_VERSIONS_ROOT
# This is included by the platform build system.
include $(CLEAR_VARS)
LOCAL_MODULE := cpufeatures
LOCAL_SRC_FILES := cpu-features.c
LOCAL_SDK_VERSION := 9
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

else # NDK build system

ifneq ($(strip $(filter-out $(NDK_KNOWN_ARCHS),$(TARGET_ARCH))),)

include $(CLEAR_VARS)
LOCAL_MODULE := cpufeatures
LOCAL_SRC_FILES := $(NDK_ROOT)/sources/android/libportable/libs/$(TARGET_ARCH_ABI)/libportable.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(PREBUILT_STATIC_LIBRARY)

else  # NDK_KNOWN_ARCHS

include $(CLEAR_VARS)
LOCAL_MODULE := cpufeatures
LOCAL_SRC_FILES := cpu-features.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)
endif # NDK_KNOWN_ARCHS

endif # HISTORICAL_NDK_VERSIONS_ROOT
