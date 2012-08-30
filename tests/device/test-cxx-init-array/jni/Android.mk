LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_cxx_init_array
LOCAL_SRC_FILES := test.cpp global_ctor.cpp
include $(BUILD_EXECUTABLE)
