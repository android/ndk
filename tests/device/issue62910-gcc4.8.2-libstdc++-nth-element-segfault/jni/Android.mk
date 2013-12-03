LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue62910-gcc4.8.2-libstdc++-nth-element-segfault
LOCAL_SRC_FILES := issue62910-gcc4.8.2-libstdc++-nth-element-segfault.cpp
LOCAL_CPPFLAGS += -std=gnu++11
include $(BUILD_EXECUTABLE)
