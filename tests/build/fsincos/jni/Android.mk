LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := fsincos
LOCAL_SRC_FILES := fsincos.c
LOCAL_CFLAGS += -fno-sincos
include $(BUILD_EXECUTABLE)
