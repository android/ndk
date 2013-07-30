LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_cc_extension_deps
LOCAL_SRC_FILES := main.cc
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_cp_extension_deps
LOCAL_SRC_FILES := main.cp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_cxx_extension_deps
LOCAL_SRC_FILES := main.cxx
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_cpp_extension_deps
LOCAL_SRC_FILES := main.cpp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_CPP_extension_deps_1
LOCAL_SRC_FILES := main.CPP
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_C_extension_deps
LOCAL_SRC_FILES := main.C
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stl_c++_extension_deps
LOCAL_SRC_FILES := main.c++
include $(BUILD_EXECUTABLE)

