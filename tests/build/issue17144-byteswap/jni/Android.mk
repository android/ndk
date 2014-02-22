LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue17144-byteswap
LOCAL_SRC_FILES := issue17144-byteswap.c
include $(BUILD_EXECUTABLE)
