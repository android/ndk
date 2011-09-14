# This is a trivial shared library that will be imported
# by 'libpath1', and hence by the project's main binary
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libpath2
LOCAL_SRC_FILES := path2.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
include $(BUILD_SHARED_LIBRARY)

