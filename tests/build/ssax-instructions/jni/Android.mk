LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := ssax_instruction
LOCAL_ARM_NEON := true
LOCAL_SRC_FILES := test.S
include $(BUILD_SHARED_LIBRARY)

