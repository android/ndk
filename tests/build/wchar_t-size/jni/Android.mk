LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_wchar_t_size
LOCAL_SRC_FILES := \
  test_default.c \
  test_8bit.c \
  test_8bit_cplusplus.cpp \
  test_8bit_cplusplus_stdc_limit_macros.cpp \
  test_always_signed.c \

include $(BUILD_SHARED_LIBRARY)
