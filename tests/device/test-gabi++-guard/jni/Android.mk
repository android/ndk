LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_guard_variables
LOCAL_SRC_FILES := test_guard.cpp
include $(BUILD_EXECUTABLE)
