LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue65705-asm-pc
LOCAL_SRC_FILES := issue65705-asm-pc.c
include $(BUILD_EXECUTABLE)
