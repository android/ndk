LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_cpufeatures
LOCAL_SRC_FILES := test_cpufeatures.c
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_EXECUTABLE)

$(call import-module,android/cpufeatures)
