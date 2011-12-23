LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gnustl_forced_all
LOCAL_SRC_FILES := forced_all.cpp
include $(BUILD_EXECUTABLE)
