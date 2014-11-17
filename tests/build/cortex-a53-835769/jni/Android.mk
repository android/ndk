LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := cortex-a53-835769-1
LOCAL_SRC_FILES := cortex-a53-835769-1.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := cortex-a53-835769-2
LOCAL_SRC_FILES := cortex-a53-835769-2.c
include $(BUILD_EXECUTABLE)

