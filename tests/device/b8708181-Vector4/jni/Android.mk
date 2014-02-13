LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := Vector4
LOCAL_SRC_FILES := Vector4.cpp
ifneq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a armeabi-v7a-hard),)
    LOCAL_CFLAGS := -DHAVE_NEON=1 -march=armv7-a -mfpu=neon -ftree-vectorize -ffast-math -mfloat-abi=softfp
endif
include $(BUILD_EXECUTABLE)
