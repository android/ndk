# This file is only used at AOSP usage. (not NDK usage)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(OUT_DIR),)
$(error No OUT_DIR defined. Not lunch yet?)
endif

COMPILER_APP_DIR := $(realpath $(OUT_DIR))/obj/APPS/AndroidBitcodeCompiler_intermediates

#######################################################

include $(CLEAR_VARS)

LOCAL_ASSET_DIR := $(COMPILER_APP_DIR)/assets  # We define this rule in jni/Android.mk
LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_PACKAGE_NAME := AndroidBitcodeCompiler
LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_ADDITIONAL_DEPENDENCIES := libjni_abcc	# Invoke building compiler app assets
LOCAL_NO_CHECK_ASSETS := true

include $(BUILD_PACKAGE)

#######################################################

include $(call all-makefiles-under,$(LOCAL_PATH))
