LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := shaderc_test
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := shaderc.cc
LOCAL_CFLAGS := -std=c++11
LOCAL_STATIC_LIBRARIES := shaderc
include $(BUILD_EXECUTABLE)

$(call import-module,third_party/shaderc)
