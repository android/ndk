LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_static
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libstdc++.a
LOCAL_EXPORT_CPPFLAGS := -fexceptions -frtti
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include \
                           $(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)
