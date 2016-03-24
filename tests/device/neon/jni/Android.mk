LOCAL_PATH := $(call my-dir)

APP_ABI         := x86 x86_64 armeabi-v7a-hard arm64-v8a

ifeq ($(TARGET_ARCH_ABI),$(filter $(TARGET_ARCH_ABI), armeabi-v7a-hard arm64-v8a x86 x86_64))
    include $(CLEAR_VARS)
    LOCAL_MODULE    := increment
    LOCAL_SRC_FILES := increment.c
    LOCAL_ARM_NEON  := true
    include $(BUILD_EXECUTABLE)
endif


