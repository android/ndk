LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_1_static
LOCAL_SRC_FILES := test_1.cc
include $(BUILD_EXECUTABLE)
