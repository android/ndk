LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := cpufeatures
LOCAL_SRC_FILES := cpu-features.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)
