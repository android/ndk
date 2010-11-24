LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_basic_exceptions
LOCAL_SRC_FILES := test_basic_exceptions.cpp
LOCAL_CPPFLAGS := -fexceptions
include $(BUILD_EXECUTABLE)
