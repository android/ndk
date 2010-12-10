# A simple module used to demonstrate multi-path NDK_MODULE_PATH imports.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := libfoo.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)
