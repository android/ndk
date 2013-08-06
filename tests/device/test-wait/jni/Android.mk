LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_wait-dynamic
LOCAL_SRC_FILES := test_wait.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_wait-static
LOCAL_SRC_FILES := test_wait.c
LOCAL_LDFLAGS += -static -Wl,--eh-frame-hdr
include $(BUILD_EXECUTABLE)

