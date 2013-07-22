LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_cpufeatures
LOCAL_SRC_FILES := test_cpufeatures.c
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_android_setCpu_1
LOCAL_SRC_FILES := test_android_setCpu_1.c
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_android_setCpu_2
LOCAL_SRC_FILES := test_android_setCpu_2.c
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_EXECUTABLE)

ifeq ($(TARGET_ARCH),arm)
include $(CLEAR_VARS)
LOCAL_MODULE := test_android_setCpuArm_1
LOCAL_SRC_FILES := test_android_setCpuArm_1.c
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_EXECUTABLE)
endif

ifeq ($(TARGET_ARCH),arm)
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
include $(CLEAR_VARS)
LOCAL_MODULE := test_arm_idiv_support
LOCAL_SRC_FILES := test_arm_idiv.c
LOCAL_CFLAGS := -mcpu=cortex-a15
# filter-out -march=armv7-a which isn't compatible with -mcpu=cortex-a15
TARGET_CFLAGS := $(filter-out -march=armv7-a, $(TARGET_CFLAGS))
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_thumb_idiv_support
LOCAL_SRC_FILES := test_arm_idiv.c
LOCAL_CFLAGS := -mcpu=cortex-a15
# filter-out -march=armv7-a which isn't compatible with -mcpu=cortex-a15
TARGET_CFLAGS := $(filter-out -march=armv7-a, $(TARGET_CFLAGS))
LOCAL_ARM_MODE := thumb
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_EXECUTABLE)
endif
endif

$(call import-module,android/cpufeatures)
