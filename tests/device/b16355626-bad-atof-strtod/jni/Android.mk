LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := b16355626-bad-atof-strtod
LOCAL_SRC_FILES := b16355626-bad-atof-strtod.cpp
LOCAL_CFLAGS += -std=c++11
include $(BUILD_EXECUTABLE)
