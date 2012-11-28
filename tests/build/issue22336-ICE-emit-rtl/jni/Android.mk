LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue22336-ICE-emit-rtl
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -O0
LOCAL_SRC_FILES := issue22336-ICE-emit-rtl.cpp
include $(BUILD_SHARED_LIBRARY)

