LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_fenv_c
LOCAL_SRC_FILES := test_fenv.c
LOCAL_LDFLAGS += -lm
include $(BUILD_EXECUTABLE)
