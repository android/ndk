LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := ucontext
LOCAL_SRC_FILES := ucontext.c
include $(BUILD_EXECUTABLE)
