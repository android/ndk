LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_basic_rtti
LOCAL_SRC_FILES := test_basic_rtti.cpp
LOCAL_CPPFLAGS := -frtti
include $(BUILD_EXECUTABLE)
