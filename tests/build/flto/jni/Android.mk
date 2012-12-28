LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := flto
LOCAL_SRC_FILES := flto.c
LOCAL_CFLAGS := -flto
LOCAL_LDFLAGS := -flto
include $(BUILD_EXECUTABLE)

