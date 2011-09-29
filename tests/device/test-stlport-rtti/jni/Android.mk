LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stlport_rtti
LOCAL_SRC_FILES := test_stlport_rtti.cpp
include $(BUILD_EXECUTABLE)
