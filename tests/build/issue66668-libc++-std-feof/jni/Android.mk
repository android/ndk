LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue66668-libc++-std-feof
LOCAL_SRC_FILES := issue66668-libc++-std-feof.cpp
include $(BUILD_EXECUTABLE)
