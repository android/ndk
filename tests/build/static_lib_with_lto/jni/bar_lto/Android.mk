LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libbar_lto

LOCAL_SRC_FILES := bar.c

include $(BUILD_STATIC_LIBRARY)
