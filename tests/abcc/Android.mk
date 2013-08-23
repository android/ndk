# This file is only used at AOSP usage. (not NDK usage)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#######################################################

include $(CLEAR_VARS)

LOCAL_ASSET_DIR := $(realpath $(LOCAL_PATH))/prebuilts/assets/$(TARGET_CPU_ABI)
LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_PACKAGE_NAME := AndroidBitcodeCompiler
LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_NO_CHECK_ASSETS := true

include $(BUILD_PACKAGE)

#######################################################

include $(call all-makefiles-under,$(LOCAL_PATH))
