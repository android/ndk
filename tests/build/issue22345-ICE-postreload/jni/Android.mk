LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(filter-out $(NDK_KNOWN_ARCHS),$(TARGET_ARCH))),)
include $(CLEAR_VARS)
LOCAL_MODULE := issue22345-ICE-postreload
LOCAL_ARM_NEON := true
LOCAL_SRC_FILES := issue22345-ICE-postreload.cpp
include $(BUILD_SHARED_LIBRARY)
endif

