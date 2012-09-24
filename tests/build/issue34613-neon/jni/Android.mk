LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue34613-neon
LOCAL_ARM_NEON := true
LOCAL_SRC_FILES := issue34613-neon.cpp
include $(BUILD_SHARED_LIBRARY)

