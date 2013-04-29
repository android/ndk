LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := foo.c
LOCAL_THIN_ARCHIVE := true
include $(BUILD_SHARED_LIBRARY)

