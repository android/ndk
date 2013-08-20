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
TOOLCHAIN_SRC_ALREADY_EXIST := true
TOOLCHAIN_SRC := /proj/mtk03872/ndk-toolchain-src-20130813

ifneq ($(ANDROID_BUILD_TOP),)
NDK_ROOT := $(ANDROID_BUILD_TOP)/ndk
else
NDK_ROOT := $(LOCAL_PATH)/../../../../ndk
endif

ifeq ($(TOOLCHAIN_SRC_ALREADY_EXIST),true)

define build_compiler_app_assets
rm -rf $(TOOLCHAIN_ASSETS)
$(NDK_ROOT)/build/tools/build-on-device-toolchain.sh --abi=$(TARGET_CPU_ABI) --out-dir=$(TOOLCHAIN_ASSETS) --no-sync $(TOOLCHAIN_SRC)
endef

else  # ($(TOOLCHAIN_SRC_ALREADY_EXIST),true)

define build_compiler_app_assets
rm -rf $(TOOLCHAIN_ASSETS)
rm -rf $(TOOLCHAIN_SRC)
mkdir -p $(TOOLCHAIN_SRC) && $(NDK_ROOT)/build/tools/download-toolchain-sources.sh $(TOOLCHAIN_SRC)
$(NDK_ROOT)/build/tools/build-on-device-toolchain.sh ---abi=$(TARGET_CPU_ABI) --out-dir=$(TOOLCHAIN_ASSETS) --no-sync $(TOOLCHAIN_SRC)
rm -rf $(TOOLCHAIN_SRC)
endef

endif

GEN := $(call local-intermediates-dir)/phony_file # Must under local intermediate dir
$(GEN):
	$(build_compiler_app_assets)
LOCAL_GENERATED_SOURCES := $(GEN)

########################################################

else  # SYSTEM_PREBUILT_PACKAGE
LOCAL_LDLIBS := -llog
endif

include $(BUILD_SHARED_LIBRARY)
