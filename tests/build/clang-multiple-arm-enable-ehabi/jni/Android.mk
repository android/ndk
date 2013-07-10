LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := clang-multiple-arm-enable-ehabi
LOCAL_SRC_FILES := clang-multiple-arm-enable-ehabi.cpp
LOCAL_CFLAGS += -mllvm -arm-enable-ehabi -mllvm -arm-enable-ehabi-descriptors
include $(BUILD_EXECUTABLE)
