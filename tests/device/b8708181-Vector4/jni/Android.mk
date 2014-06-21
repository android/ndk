LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := Vector4
LOCAL_SRC_FILES := Vector4.cpp
ifneq ($(filter $(TARGET_ARCH_ABI), armeabi-v7a armeabi-v7a-hard),)
    LOCAL_CFLAGS += -DHAVE_NEON=1 -march=armv7-a -mfpu=neon -ftree-vectorize -ffast-math -mfloat-abi=softfp
endif
ifneq ($(filter $(TARGET_ARCH_ABI),x86),)
# x86 ABI was recently changed to gen SSSE3 by default. Disable it in order
# for this test to run on emulator-x86 w/o KVM/HAXM
    LOCAL_CFLAGS += -mno-ssse3 -msse3
endif

include $(BUILD_EXECUTABLE)
