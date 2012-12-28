LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue39680-chrono-resolution
LOCAL_SRC_FILES := issue39680-chrono-resolution.cpp
LOCAL_CFLAGS += -std=gnu++0x
include $(BUILD_EXECUTABLE)

