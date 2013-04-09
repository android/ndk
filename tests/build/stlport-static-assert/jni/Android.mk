LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := stlport-static-assert
LOCAL_SRC_FILES := stlport-static-assert.cpp
include $(BUILD_EXECUTABLE)
