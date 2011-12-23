LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gnustl_forced_exceptions
LOCAL_SRC_FILES := forced_exceptions.cpp
include $(BUILD_EXECUTABLE)

