LOCAL_PATH := $(call my-dir)

# Module declaration for the 'minitest' unit-test
# library. Note that it provides a main() implementation.
include $(CLEAR_VARS)
LOCAL_MODULE := minitest
LOCAL_SRC_FILES := minitest/minitest.cc
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_STATIC_LIBRARY)

# Module declaration for the unit test program.
include $(CLEAR_VARS)
LOCAL_MODULE := android_support_unittests
LOCAL_SRC_FILES := \
  ctype_unittest.cc \
  math_unittest.cc \
  stdio_unittest.cc \
  wchar_unittest.cc

ifeq ($(filter $(NDK_KNOWN_DEVICE_ABI64S),$(TARGET_ARCH_ABI)),)
LOCAL_SRC_FILES += \
  libdl_unittest.cc
endif

LOCAL_STATIC_LIBRARIES := android_support minitest
include $(BUILD_EXECUTABLE)

# Include the android_support module definitions.
include $(LOCAL_PATH)/../Android.mk

