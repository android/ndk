LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue46718-iostream-crash-gnustl
LOCAL_SRC_FILES := issue46718-iostream-crash.cpp
include $(BUILD_EXECUTABLE)

