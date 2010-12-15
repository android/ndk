LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_inet_defs
LOCAL_SRC_FILES := test-in.c test-in6.c
include $(BUILD_SHARED_LIBRARY)
