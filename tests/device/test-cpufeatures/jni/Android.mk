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

$(call import-module,android/cpufeatures)
