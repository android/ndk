LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue-gcc59052-partial-specialization-of-template
LOCAL_SRC_FILES := issue-gcc59052-partial-specialization-of-template.cc
include $(BUILD_EXECUTABLE)
