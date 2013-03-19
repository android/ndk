LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := stat
LOCAL_SRC_FILES := stat.c
include $(BUILD_EXECUTABLE)
