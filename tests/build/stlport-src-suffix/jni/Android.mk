LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stlport
LOCAL_SRC_FILES := test_stlport.cpp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stlport_arm
LOCAL_SRC_FILES := test_stlport.cpp.arm
include $(BUILD_EXECUTABLE)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stlport_neon
LOCAL_SRC_FILES := test_stlport.cpp.neon
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_stlport_arm_neon
LOCAL_SRC_FILES := test_stlport.cpp.arm.neon
include $(BUILD_EXECUTABLE)

endif