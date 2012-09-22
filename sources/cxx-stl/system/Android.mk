LOCAL_PATH:= $(call my-dir)

ifneq ($(NDK_APP_USE_BITCODE),true)

include $(CLEAR_VARS)

LOCAL_MODULE := libstdc++
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_LDLIBS := -lstdc++

include $(BUILD_STATIC_LIBRARY)

else # NDK_APP_USE_BITCODE == true

include $(CLEAR_VARS)

LOCAL_MODULE := libstdc++
LOCAL_SRC_FILES := ../../../platforms/$(TARGET_PLATFORM)/arch-arm/usr/lib/libstdc++.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_EXPORT_LDLIBS := -lstdc++

include $(PREBUILT_SHARED_LIBRARY)

endif # NDK_APP_USE_BITCODE == true

