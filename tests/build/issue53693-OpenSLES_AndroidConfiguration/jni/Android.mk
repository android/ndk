LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue53693-OpenSLES_AndroidConfiguration
LOCAL_SRC_FILES := issue53693-OpenSLES_AndroidConfiguration.c
include $(BUILD_EXECUTABLE)
