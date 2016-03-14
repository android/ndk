LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := pch
LOCAL_PCH := stdafx.h
LOCAL_SRC_FILES := foo.cpp bar.cpp
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := pch-arm
LOCAL_PCH := stdafx.h
LOCAL_SRC_FILES := foo.cpp bar.cpp.arm
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := pch-neon
LOCAL_PCH := stdafx.h
LOCAL_SRC_FILES := foo.cpp bar.cpp.neon
include $(BUILD_SHARED_LIBRARY)
