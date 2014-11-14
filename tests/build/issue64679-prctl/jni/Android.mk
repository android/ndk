LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue64679-prctl
LOCAL_SRC_FILES := issue64679-prctl.cc
LOCAL_CPPFLAGS += -std=c++11
include $(BUILD_EXECUTABLE)
