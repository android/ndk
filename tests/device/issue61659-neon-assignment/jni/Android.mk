LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue61659-neon-assignment
LOCAL_SRC_FILES := issue61659-neon-assignment.c.neon
include $(BUILD_EXECUTABLE)
