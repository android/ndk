LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue38121_main
LOCAL_SRC_FILES := main.c
LOCAL_CFLAGS += -fstack-protector-all -static
include $(BUILD_EXECUTABLE)
