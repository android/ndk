LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(filter-out $(NDK_KNOWN_ARCHS),$(TARGET_ARCH))),)
include $(CLEAR_VARS)
LOCAL_MODULE := emm
LOCAL_SRC_FILES := emm.c
include $(BUILD_EXECUTABLE)
endif
