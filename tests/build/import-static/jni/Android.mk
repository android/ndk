LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_static_imports
LOCAL_SRC_FILES := main.c
LOCAL_STATIC_LIBRARIES := libfoo
include $(BUILD_SHARED_LIBRARY)

$(call import-module,foo)

