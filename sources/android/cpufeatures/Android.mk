LOCAL_PATH := $(call my-dir)

ifneq ($(strip $(filter-out $(NDK_KNOWN_ARCHS),$(TARGET_ARCH))),)

include $(CLEAR_VARS)
LOCAL_MODULE := cpufeatures
LOCAL_SRC_FILES := $(NDK_ROOT)/sources/android/libportable/libs/$(TARGET_ARCH_ABI)/libportable.a

ifdef HISTORICAL_NDK_VERSIONS_ROOT
# This is included by the platform build system.
LOCAL_SDK_VERSION := 9
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
else
# This is included by the NDK build system.
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
endif

include $(PREBUILT_STATIC_LIBRARY)

else

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

endif
