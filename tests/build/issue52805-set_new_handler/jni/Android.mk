LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := new_handler
LOCAL_SRC_FILES := new_handler.cpp
include $(BUILD_SHARED_LIBRARY)
