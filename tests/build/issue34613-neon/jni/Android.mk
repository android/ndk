LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(filter-out $(NDK_KNOWN_ARCHS),$(TARGET_ARCH))),)
include $(CLEAR_VARS)
LOCAL_MODULE := issue34613-neon
LOCAL_ARM_NEON := true
LOCAL_SRC_FILES := issue34613-neon.cpp
include $(BUILD_SHARED_LIBRARY)
endif
