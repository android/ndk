LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue38441-Elf32_auxv_t
LOCAL_SRC_FILES := issue38441-Elf32_auxv_t.c
include $(BUILD_EXECUTABLE)
