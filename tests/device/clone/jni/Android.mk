LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := clone
LOCAL_SRC_FILES := clone.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := clone-static
LOCAL_SRC_FILES := clone.c
LOCAL_LDFLAGS += -static -Wl,--eh-frame-hdr
include $(BUILD_EXECUTABLE)
