LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_exception_crash
LOCAL_SRC_FILES := main.cpp
LOCAL_CPP_FEATURES := exceptions
include $(BUILD_EXECUTABLE)
