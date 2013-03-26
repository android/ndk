LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := deprecate-__set_errno
LOCAL_SRC_FILES := deprecate-__set_errno.c foo.c
LOCAL_CFLAGS += -Werror
include $(BUILD_EXECUTABLE)
