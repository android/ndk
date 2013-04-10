LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := stlport-static-assert
LOCAL_SRC_FILES := stlport-static-assert.cpp
LOCAL_CFLAGS += -Wall -Werror
include $(BUILD_EXECUTABLE)
