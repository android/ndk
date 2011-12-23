LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gnustl_forced_rtti
LOCAL_SRC_FILES := forced_rtti.cpp
include $(BUILD_EXECUTABLE)
