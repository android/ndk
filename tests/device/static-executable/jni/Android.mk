LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := static_main
LOCAL_SRC_FILES := main.c
LOCAL_LDFLAGS := -static  # this forces the generation of a static executable
include $(BUILD_EXECUTABLE)

