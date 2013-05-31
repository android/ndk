LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := b9193874-neon
LOCAL_SRC_FILES := b9193874-neon.c.neon
LOCAL_CFLAGS += -frename-registers -O2
include $(BUILD_SHARED_LIBRARY)
