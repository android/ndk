# Copyright (C) 2010 The Android Open Source Project
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

# this file is included from Android.mk files to build a target-specific
# shared library
#

LOCAL_BUILD_SCRIPT := PREBUILT_SHARED_LIBRARY
LOCAL_MAKEFILE     := $(local-makefile)

$(call check-defined-LOCAL_MODULE,$(LOCAL_BUILD_SCRIPT))
$(call check-LOCAL_MODULE,$(LOCAL_MAKEFILE))
$(call check-LOCAL_MODULE_FILENAME)

# Check that LOCAL_SRC_FILES contains only paths to shared libraries
ifneq ($(words $(LOCAL_SRC_FILES)),1)
$(call __ndk_info,ERROR:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): The LOCAL_SRC_FILES for a prebuilt shared library should only contain one item))
$(call __ndk_error,Aborting)
endif

bad_prebuilts := $(filter-out %.so,$(LOCAL_SRC_FILES))
ifdef bad_prebuilts
$(call __ndk_info,ERROR:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): LOCAL_SRC_FILES should point to a prebuilt shared library)
$(call __ndk_info,The following files are unsupported: $(bad_prebuilts))
$(call __ndk_error,Aborting)
endif

prebuilt := $(strip $(wildcard $(LOCAL_SRC_FILES)))
ifndef prebuilt
$(call __ndk_info,ERROR:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): LOCAL_SRC_FILES points to a missing file)
$(call __ndk_info,Check that $(LOCAL_SRC_FILES) exists, or that its path is correct)
$(call __ndk_error,Aborting)
endif

ifdef LOCAL_MODULE_FILENAME
$(call __ndk_info,ERROR:$(LOCAL_MAKEFILE):$(LOCAL_MODULE): LOCAL_MODULE_FILENAME cannot be used for a prebuilt shared library)
$(call __ndk_error,Aborting)
endif

LOCAL_BUILT_MODULE := $(LOCAL_SRC_FILES)
LOCAL_SRC_FILES    :=
LOCAL_OBJS_DIR     := $(TARGET_OBJS)/$(LOCAL_MODULE)

LOCAL_MODULE_CLASS := PREBUILT_SHARED_LIBRARY
include $(BUILD_SYSTEM)/build-module.mk
