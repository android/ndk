LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := sample.c sample_lib.c
LOCAL_MODULE := sample
LOCAL_ARM_MODE := arm

include $(BUILD_EXECUTABLE)
