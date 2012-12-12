LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_chrono
LOCAL_SRC_FILES := main.cpp hanoi.c
LOCAL_CPPFLAGS := -std=c++0x
include $(BUILD_EXECUTABLE)
