LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue35933-lambda
LOCAL_SRC_FILES := issue35933-lambda.cpp
LOCAL_CFLAGS += -std=gnu++0x
include $(BUILD_EXECUTABLE)
