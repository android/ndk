LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue41297-atomic-64bit-dynamic-executable
LOCAL_SRC_FILES := issue41297-atomic-64bit.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := issue41297-atomic-64bit-static-executable
LOCAL_SRC_FILES := issue41297-atomic-64bit.c
LOCAL_LDFLAGS += -static
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := issue41297-atomic-64bit-shared-lib
LOCAL_SRC_FILES := issue41297-atomic-64bit.c
include $(BUILD_SHARED_LIBRARY)
