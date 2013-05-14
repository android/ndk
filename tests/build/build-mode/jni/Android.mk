LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(filter-out $(NDK_KNOWN_ARCHS),$(TARGET_ARCH))),)

# We build up to 4 armeabi binaries
# To check for thumb/arm build modes, either with the .arm extension
# or using LOCAL_ARM_MODE
#
ifeq ($(TARGET_ARCH_ABI),armeabi)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_thumb
LOCAL_CFLAGS += -DCHECK_THUMB
LOCAL_SRC_FILES := main.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_thumb_b
LOCAL_CFLAGS += -DCHECK_THUMB
LOCAL_SRC_FILES := main.c
LOCAL_ARM_MODE := thumb
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_arm
LOCAL_CFLAGS += -DCHECK_ARM
LOCAL_SRC_FILES := main.c.arm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_arm_b
LOCAL_CFLAGS += -DCHECK_ARM
LOCAL_SRC_FILES := main.c
LOCAL_ARM_MODE := arm
include $(BUILD_EXECUTABLE)

endif # TARGET_ARCH == arm

# We build 8 armeabi-v7a binaries because we need to check neon as well
#
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_thumb2
LOCAL_CFLAGS += -DCHECK_THUMB2
LOCAL_SRC_FILES := main.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_thumb2_b
LOCAL_CFLAGS += -DCHECK_THUMB2
LOCAL_SRC_FILES := main.c
LOCAL_ARM_MODE := thumb
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_armv7
LOCAL_CFLAGS += -DCHECK_ARM
LOCAL_SRC_FILES := main.c.arm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_armv7_b
LOCAL_CFLAGS += -DCHECK_ARM
LOCAL_SRC_FILES := main.c
LOCAL_ARM_MODE := arm
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_thumb2_neon
LOCAL_CFLAGS += -DCHECK_THUMB2 -DCHECK_NEON
LOCAL_SRC_FILES := main.c.neon
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_thumb2_neon_b
LOCAL_CFLAGS += -DCHECK_THUMB2 -DCHECK_NEON
LOCAL_SRC_FILES := main.c
LOCAL_ARM_NEON := true
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_thumb2_neon_c
LOCAL_CFLAGS += -DCHECK_THUMB2 -DCHECK_NEON
LOCAL_SRC_FILES := main.c
LOCAL_ARM_MODE := thumb
LOCAL_ARM_NEON := true
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_armv7_neon
LOCAL_CFLAGS += -DCHECK_ARM -DCHECK_NEON
LOCAL_SRC_FILES := main.c.arm.neon
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_armv7_neon_b
LOCAL_CFLAGS += -DCHECK_ARM -DCHECK_NEON
LOCAL_SRC_FILES := main.c.arm
LOCAL_ARM_NEON := true
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_armv7_neon_c
LOCAL_CFLAGS += -DCHECK_ARM -DCHECK_NEON
LOCAL_SRC_FILES := main.c
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_EXECUTABLE)

endif # TARGET_ARCH_ABI == armeabi-v7a

# We only build a single binary for x86
#
ifeq ($(TARGET_ARCH),x86)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_x86
LOCAL_CFLAGS += -DCHECK_X86
LOCAL_SRC_FILES := main.c
include $(BUILD_EXECUTABLE)

endif # TARGET_ARCH == x86

ifeq ($(TARGET_ARCH),mips)

include $(CLEAR_VARS)
LOCAL_MODULE := test_build_mode_mips
LOCAL_CFLAGS += -DCHECK_MIPS
LOCAL_SRC_FILES := main.c
include $(BUILD_EXECUTABLE)

endif # TARGET_ARCH == mips

endif # if TARGET_ARCH is known arch
