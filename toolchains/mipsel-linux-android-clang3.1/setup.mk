# Copyright (C) 2012 The Android Open Source Project
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

# this file is used to prepare the NDK to build with the mipsel llvm-3.1
# toolchain any number of source files
#
# its purpose is to define (or re-define) templates used to build
# various sources into target object files, libraries or executables.
#
# Note that this file may end up being parsed several times in future
# revisions of the NDK.
#

#
# Override the toolchain prefix
#

LLVM_VERSION := 3.1
LLVM_NAME := llvm-$(LLVM_VERSION)
LLVM_TOOLCHAIN_ROOT := $(NDK_ROOT)/toolchains/$(LLVM_NAME)
LLVM_TOOLCHAIN_PREBUILT_ROOT := $(LLVM_TOOLCHAIN_ROOT)/prebuilt/$(HOST_TAG)
LLVM_TOOLCHAIN_PREFIX := $(LLVM_TOOLCHAIN_PREBUILT_ROOT)/bin/

TOOLCHAIN_VERSION := 4.6
TOOLCHAIN_NAME := mipsel-linux-android-$(TOOLCHAIN_VERSION)
TOOLCHAIN_ROOT := $(NDK_ROOT)/toolchains/$(TOOLCHAIN_NAME)
TOOLCHAIN_PREBUILT_ROOT := $(TOOLCHAIN_ROOT)/prebuilt/$(HOST_TAG)
TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREBUILT_ROOT)/bin/mipsel-linux-android-

TARGET_CC := $(LLVM_TOOLCHAIN_PREFIX)clang
TARGET_CXX := $(LLVM_TOOLCHAIN_PREFIX)clang++

#
# CFLAGS, C_INCLUDES, and LDFLAGS
#

LLVM_TRIPLE := mipsel-none-linux-android

TARGET_CFLAGS := \
        -B$(TOOLCHAIN_PREBUILT_ROOT) \
        -target $(LLVM_TRIPLE) \
        -fpic \
        -fno-strict-aliasing \
        -finline-functions \
        -ffunction-sections \
        -funwind-tables \
        -fmessage-length=0

TARGET_LDFLAGS := \
        -B$(TOOLCHAIN_PREBUILT_ROOT) \
        -target $(LLVM_TRIPLE)

TARGET_C_INCLUDES := \
    $(SYSROOT)/usr/include

TARGET_mips_release_CFLAGS := -O2 \
                              -fomit-frame-pointer

TARGET_mips_debug_CFLAGS := -O0 -g \
                            -fno-omit-frame-pointer


# This function will be called to determine the target CFLAGS used to build
# a C or Assembler source file, based on its tags.
TARGET-process-src-files-tags = \
$(eval __debug_sources := $(call get-src-files-with-tag,debug)) \
$(eval __release_sources := $(call get-src-files-without-tag,debug)) \
$(call set-src-files-target-cflags, \
    $(__debug_sources),\
    $(TARGET_mips_debug_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(__release_sources),\
    $(TARGET_mips_release_CFLAGS)) \
$(call set-src-files-text,$(__debug_sources),mips$(space)) \
$(call set-src-files-text,$(__release_sources),mips$(space)) \
