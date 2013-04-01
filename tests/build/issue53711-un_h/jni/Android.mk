LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue53711-un_h
LOCAL_SRC_FILES := issue53711-un_h.c
include $(BUILD_EXECUTABLE)
