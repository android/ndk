LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue19851-sigsetjmp
LOCAL_SRC_FILES := issue19851-sigsetjmp.c
include $(BUILD_EXECUTABLE)
