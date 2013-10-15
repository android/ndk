LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := math_c
LOCAL_SRC_FILES := math.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := math_cpp
LOCAL_SRC_FILES := math.cpp
include $(BUILD_EXECUTABLE)
