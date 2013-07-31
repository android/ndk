LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := lambda-defarg3
LOCAL_SRC_FILES := lambda-defarg3.C
LOCAL_CFLAGS += -std=c++11 -fabi-version=0
include $(BUILD_EXECUTABLE)
