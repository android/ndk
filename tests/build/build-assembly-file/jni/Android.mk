LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test-build-assembly
LOCAL_SRC_FILES := assembly1.s assembly2.S
include $(BUILD_SHARED_LIBRARY)
