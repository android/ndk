LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := foo.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_STATIC_LIBRARIES := libbar
include $(BUILD_STATIC_LIBRARY)

$(call import-module,bar)

