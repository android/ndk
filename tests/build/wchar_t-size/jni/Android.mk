LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_wchar_t_size
LOCAL_SRC_FILES := \
  test_default.c \
  test_always_signed.c \

include $(BUILD_SHARED_LIBRARY)
