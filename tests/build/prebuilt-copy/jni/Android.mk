LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := $(PREBUILTS_DIR)/libs/*$(TARGET_ARCH_ABI)/libfoo.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libbar
LOCAL_SRC_FILES := $(PREBUILTS_DIR)/obj/local/*$(TARGET_ARCH_ABI)/libbar.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test_prebuilts_copy
LOCAL_SRC_FILES := main.c
LOCAL_SHARED_LIBRARIES := libfoo
LOCAL_STATIC_LIBRARIES := libbar
include $(BUILD_EXECUTABLE)
