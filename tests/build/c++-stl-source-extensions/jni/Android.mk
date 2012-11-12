LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_cpp_extension_dependencies
LOCAL_SRC_FILES := main.cc
include $(BUILD_EXECUTABLE)
