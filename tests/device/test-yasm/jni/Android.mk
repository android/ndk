LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test-yasm
LOCAL_SRC_FILES := \
   test-yasm.c \
   print_hello.asm
include $(BUILD_EXECUTABLE)
