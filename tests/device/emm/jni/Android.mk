LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := emm
LOCAL_SRC_FILES := emm.c
include $(BUILD_EXECUTABLE)

