LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := clang-include-gnu-libc++
LOCAL_SRC_FILES := clang-include-gnu-libc++.cc
LOCAL_CPPFLAGS += -std=gnu++11
include $(BUILD_EXECUTABLE)
