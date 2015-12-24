LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := copysign
LOCAL_SRC_FILES := copysign.cpp
LOCAL_CPPFLAGS := -std=c++11
include $(BUILD_SHARED_LIBRARY)
