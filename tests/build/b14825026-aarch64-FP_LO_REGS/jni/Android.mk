LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := idct_dequant_full_2x_neon
LOCAL_SRC_FILES := idct_dequant_full_2x_neon.c
include $(BUILD_EXECUTABLE)
