JNI_PATH := $(call my-dir)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := static_lib_with_lto

LOCAL_SRC_FILES := foo.c

LOCAL_STATIC_LIBRARIES :=  libbar_lto

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/bar_lto/Android.mk
