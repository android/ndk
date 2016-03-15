LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := pch
LOCAL_PCH := stdafx.h
LOCAL_SRC_FILES := foo.cpp bar.cpp
LOCAL_CFLAGS := -Werror
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := pch-arm
LOCAL_PCH := stdafx.h
LOCAL_SRC_FILES := foo.cpp bar.cpp.arm
LOCAL_CFLAGS := -Werror
include $(BUILD_SHARED_LIBRARY)

# armeabi doesn't support neon, but we want to build the rest of the test for
# that ABI.
ifneq ($(TARGET_ARCH_ABI),armeabi)
include $(CLEAR_VARS)
LOCAL_MODULE := pch-neon
LOCAL_PCH := stdafx.h
LOCAL_SRC_FILES := foo.cpp bar.cpp.neon
LOCAL_CFLAGS := -Werror
include $(BUILD_SHARED_LIBRARY)
endif
