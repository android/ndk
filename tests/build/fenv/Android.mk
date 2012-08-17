LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_fenv
LOCAL_SRC_FILES := test_fenv.c
include $(BUILD_EXECUTABLE)
