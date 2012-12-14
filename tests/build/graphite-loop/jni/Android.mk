LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE :=  graphite-loop-transformation
LOCAL_SRC_FILES := graphite-loop-transformation.c
LOCAL_CFLAGS += -floop-interchange
include $(BUILD_EXECUTABLE)
