LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_compiler_bug1
LOCAL_SRC_FILES := main.c
include $(BUILD_EXECUTABLE)
