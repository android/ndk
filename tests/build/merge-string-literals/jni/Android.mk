LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := merge-string-literals
LOCAL_SRC_FILES := merge-string-literals-1.c merge-string-literals-2.c
include $(BUILD_SHARED_LIBRARY)
