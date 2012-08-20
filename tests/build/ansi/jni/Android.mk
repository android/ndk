LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_ansi_c
LOCAL_SRC_FILES := test_ansi.c
LOCAL_CFLAGS += -ansi
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_ansi_cpp
LOCAL_SRC_FILES := test_ansi.cpp
LOCAL_CFLAGS += -ansi
include $(BUILD_EXECUTABLE)
