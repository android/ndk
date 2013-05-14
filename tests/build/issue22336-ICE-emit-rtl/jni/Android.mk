LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(filter-out $(NDK_KNOWN_ARCHS),$(TARGET_ARCH))),)
include $(CLEAR_VARS)
LOCAL_MODULE := issue22336-ICE-emit-rtl
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -O0
LOCAL_SRC_FILES := issue22336-ICE-emit-rtl.cpp
include $(BUILD_SHARED_LIBRARY)
endif
