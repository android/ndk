LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue21132-__ARM_ARCH__
LOCAL_SRC_FILES := issue21132-__ARM_ARCH__.c
include $(BUILD_EXECUTABLE)

