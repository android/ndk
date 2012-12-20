LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue20176-__gnu_Unwind_Find_exidx
LOCAL_SRC_FILES := issue20176-__gnu_Unwind_Find_exidx.cpp
include $(BUILD_EXECUTABLE)
