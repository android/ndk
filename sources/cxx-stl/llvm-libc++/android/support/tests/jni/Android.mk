LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android_support_unittests
LOCAL_SRC_FILES := \
  ../stdio_unittest.cc \
  ../wchar_unittest.cc \

LOCAL_STATIC_LIBRARIES := llvm_libc++_support_android minitest

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := minitest
LOCAL_SRC_FILES := ../minitest/minitest.cc
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/..
include $(BUILD_STATIC_LIBRARY)

include $(LOCAL_PATH)/../../Android.mk

