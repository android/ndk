LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := my_jni_module
LOCAL_SRC_FILES := my_jni_module.cpp
include $(BUILD_SHARED_LIBRARY)
