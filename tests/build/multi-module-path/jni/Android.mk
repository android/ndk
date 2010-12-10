LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := multi_path_test
LOCAL_SRC_FILES := test.c
LOCAL_STATIC_LIBRARIES := libbar libfoo
include $(BUILD_EXECUTABLE)

$(call import-module,bar)
