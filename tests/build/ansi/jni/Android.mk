LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_ansi_c
LOCAL_SRC_FILES := test_ansi.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_ansi_cpp
LOCAL_SRC_FILES := test_ansi.cpp
include $(BUILD_EXECUTABLE)
