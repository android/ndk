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
LOCAL_C_INCLUDES := $(LOCAL_PATH)/device
LOCAL_CFLAGS += -DON_DEVICE=1
LOCAL_SRC_FILES := \
  Abcc.cpp \
  device/Abcc_device.cpp \
  device/main.cpp

# Check in AOSP or NDK usage
ifeq ($(SYSTEM_PREBUILT_PACKAGE),true)

LOCAL_SHARED_LIBRARIES := liblog libstlport
include external/stlport/libstlport.mk

include $(BUILD_SHARED_LIBRARY)
else  # SYSTEM_PREBUILT_PACKAGE

#FIXME: parallel llvm codegen is not yet ported to llvm-3.5
#LOCAL_CFLAGS += -DENABLE_PARALLEL_LLVM_CG=1
LOCAL_CFLAGS += -DVERBOSE=0
LOCAL_LDLIBS := -llog
LOCAL_STATIC_LIBRARIES := cpufeatures
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)

endif

