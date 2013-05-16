LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue54623-dcraw_common-x86-segfault
LOCAL_SRC_FILES := issue54623-dcraw_common-x86-segfault.cpp
include $(BUILD_EXECUTABLE)
