LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := awk-trailing-r
LOCAL_SRC_FILES := awk-trailing-r.c
include $(BUILD_SHARED_LIBRARY)
