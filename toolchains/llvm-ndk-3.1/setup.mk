# Copyright (C) 2009 The Android Open Source Project
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

# this file is used to prepare the NDK to build with the llvm-ndk
# toolchain any number of source files
#
# its purpose is to define (or re-define) templates used to build
# various sources into target object files, libraries or executables.
#
# Note that this file may end up being parsed several times in future
# revisions of the NDK.
#

TOOLCHAIN_NAME   := llvm-ndk-3.1

LLVM_VERSION := 3.1
LLVM_NAME := llvm-$(LLVM_VERSION)
LLVM_TOOLCHAIN_ROOT := $(NDK_ROOT)/toolchains/$(LLVM_NAME)
LLVM_TOOLCHAIN_PREBUILT_ROOT := $(LLVM_TOOLCHAIN_ROOT)/prebuilt/$(HOST_TAG)
LLVM_TOOLCHAIN_PREFIX := $(LLVM_TOOLCHAIN_PREBUILT_ROOT)/bin/

# overwrite default toolchain settings
TARGET_CC       := $(TOOLCHAIN_PREBUILT_ROOT)/bin/llvm-ndk-cc
TARGET_CXX      := $(TOOLCHAIN_PREBUILT_ROOT)/bin/llvm-ndk-cxx
TARGET_LD       := $(TOOLCHAIN_PREBUILT_ROOT)/bin/llvm-ndk-link
TARGET_AR       := $(LLVM_TOOLCHAIN_PREFIX)llvm-ar
TARGET_STRIP    := $(TOOLCHAIN_PREBUILT_ROOT)/bin/llvm-ndk-link -strip-all

TARGET_CFLAGS   := -emit-llvm -nostdlibinc
TARGET_CXXFLAGS := $(TARGET_CFLAGS) -fno-exceptions -fno-rtti

ifeq ($(APP_OPTIM),debug)
TARGET_LDFLAGS  :=
else
TARGET_LDFLAGS  := -strip-all
endif

TARGET_LDLIBS   := -lc -lm -lportable

# Use ARM's headers
TARGET_C_INCLUDES :=  $(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/include

TARGET_C_LIB      :=  $(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/lib

# -Wa,--noexecstack is not used in llvm-ndk-cc
TARGET_NO_EXECUTE_CFLAGS :=

# Add and LDFLAGS for the target here
# TARGET_LDFLAGS :=

TARGET_llvm_release_CFLAGS :=

# When building for debug, compile everything as pndk.
TARGET_llvm_debug_CFLAGS := $(TARGET_pndk_release_CFLAGS)

# This function will be called to determine the target CFLAGS used to build
# a C or Assembler source file, based on its tags.
#
TARGET-process-src-files-tags = \
$(eval __debug_sources := $(call get-src-files-with-tag,debug)) \
$(eval __release_sources := $(call get-src-files-without-tag,debug)) \
$(call set-src-files-target-cflags, $(__debug_sources), $(TARGET_llvm_debug_CFLAGS)) \
$(call set-src-files-target-cflags, $(__release_sources),$(TARGET_llvm_release_CFLAGS)) \
$(call set-src-files-text,$(LOCAL_SRC_FILES),llvm$(space)$(space)) \

# The ABI-specific sub-directory that the SDK tools recognize for
# this toolchain's generated binaries
TARGET_ABI_SUBDIR := llvm

define cmd-build-shared-library
$(PRIVATE_LD) \
    $(TARGET_LDFLAGS) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(filter-out %.a,$(PRIVATE_LDLIBS)) \
    -o $(call host-path, $(LOCAL_BUILT_MODULE))
endef

define cmd-build-executable
$(PRIVATE_LD) \
    $(TARGET_LDFLAGS) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(filter-out %.a,$(PRIVATE_LDLIBS)) \
    -o $(call host-path, $(LOCAL_BUILT_MODULE))
endef

define cmd-build-static-library
$(PRIVATE_LD) \
    $(TARGET_LDFLAGS) \
    $(PRIVATE_AR_OBJECTS) \
    -o $(call host-path,$(LOCAL_BUILT_MODULE))
endef

cmd-strip = \
