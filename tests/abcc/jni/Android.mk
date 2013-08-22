# Copyright (C) 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

SYSTEM_PREBUILT_PACKAGE ?= true

LOCAL_MODULE    := libjni_abcc
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := main.cpp

## Read ldflags from bitcode wrapper or info file
#LOCAL_CFLAGS    := -DREAD_BITCODE_INFO_FILE

# Check in AOSP or NDK usage
ifeq ($(SYSTEM_PREBUILT_PACKAGE),true)

ifeq ($(OUT_DIR),)
$(error No OUT_DIR defined. Not lunch yet?)
endif

COMPILER_APP_DIR := $(realpath $(OUT_DIR))/obj/APPS/AndroidBitcodeCompiler_intermediates

LOCAL_SHARED_LIBRARIES := liblog libstlport
include external/stlport/libstlport.mk

#######################################################
# We put this here since LOCAL_GENERATED_SOURCES only been used by binary.mk

TOOLCHAIN_ASSETS := $(COMPILER_APP_DIR)/assets

ifndef NDK_TARGET_SYSROOT
NDK_TARGET_SYSROOT := $(LOCAL_PATH)/../prebuilts/assets
endif

GEN := $(call local-intermediates-dir)/phony_file # Must under local intermediate dir
$(GEN):
	$(hide) rm -rf $(TOOLCHAIN_ASSETS)
	$(hide) mkdir -p $(TOOLCHAIN_ASSETS)
	$(hide) cp -a $(NDK_TARGET_SYSROOT)/$(TARGET_CPU_ABI)/* $(TOOLCHAIN_ASSETS)

LOCAL_GENERATED_SOURCES := $(GEN)

include $(BUILD_SHARED_LIBRARY)

########################################################
else  # SYSTEM_PREBUILT_PACKAGE

LOCAL_CFLAGS += -DENABLE_PARALLEL_LLVM_CG=1
LOCAL_LDLIBS := -llog
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)

endif

