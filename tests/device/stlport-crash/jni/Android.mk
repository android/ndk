LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stlport_crash
LOCAL_SRC_FILES := main.cpp
include $(BUILD_EXECUTABLE)
