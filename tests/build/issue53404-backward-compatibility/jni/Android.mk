LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue53404-backward-compatibility
LOCAL_SRC_FILES := issue53404-backward-compatibility.cpp
LOCAL_CFLAGS := -Wno-deprecated
include $(BUILD_EXECUTABLE)
