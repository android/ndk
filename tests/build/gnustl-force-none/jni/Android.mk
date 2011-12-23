LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gnustl_force_none
LOCAL_SRC_FILES := force_none.cpp
include $(BUILD_EXECUTABLE)
