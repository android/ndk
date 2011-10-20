LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := foo.cpp
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gnustl_shared_link
LOCAL_SRC_FILES := main.cpp
LOCAL_SHARED_LIBRARIES := libfoo
include $(BUILD_EXECUTABLE)
