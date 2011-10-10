LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_static
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libgnustl_static.a
LOCAL_EXPORT_CPPFLAGS := -fexceptions -frtti
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include \
                           $(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_shared
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libgnustl_shared.so
LOCAL_EXPORT_CPPFLAGS := -fexceptions -frtti
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include \
                           $(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_SHARED_LIBRARY)
