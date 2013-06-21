LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue56508-gcc4.7-ICE
LOCAL_SRC_FILES := extraMachine.c
include $(BUILD_EXECUTABLE)
