LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := alias1
LOCAL_SRC_FILES := alias1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
