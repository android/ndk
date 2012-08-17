LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_fenv_c
LOCAL_SRC_FILES := test_fenv.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_fenv_cpp
LOCAL_SRC_FILES := test_fenv.cpp
include $(BUILD_EXECUTABLE)
