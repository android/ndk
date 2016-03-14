LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := pch-cpp
LOCAL_PCH := stdafx.h
LOCAL_SRC_FILES := foo.cpp
LOCAL_CFLAGS := -Werror
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := pch-c
LOCAL_PCH := stdafx.h
LOCAL_SRC_FILES := foo.c
LOCAL_CFLAGS := -Werror
include $(BUILD_SHARED_LIBRARY)
