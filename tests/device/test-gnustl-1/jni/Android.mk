LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gnustl_1
LOCAL_SRC_FILES := hello.cpp
include $(BUILD_EXECUTABLE)
