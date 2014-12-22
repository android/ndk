LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue81440-non-ascii-comment
LOCAL_SRC_FILES := issue81440-non-ascii-comment.cpp
LOCAL_CFLAGS += -fmessage-length=150
include $(BUILD_EXECUTABLE)
