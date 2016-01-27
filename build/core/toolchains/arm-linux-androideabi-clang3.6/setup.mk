# Copyright (C) 2014 The Android Open Source Project
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

# this file is used to prepare the NDK to build with the arm clang toolchain any
# number of source files
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

LLVM_TOOLCHAIN_PREBUILT_ROOT := $(call get-toolchain-root,llvm)
LLVM_TOOLCHAIN_PREFIX := $(LLVM_TOOLCHAIN_PREBUILT_ROOT)/bin/

TOOLCHAIN_NAME := arm-linux-androideabi
BINUTILS_ROOT := $(call get-binutils-root,$(NDK_ROOT),$(TOOLCHAIN_NAME))
TOOLCHAIN_ROOT := $(call get-toolchain-root,$(TOOLCHAIN_NAME)-4.9)
TOOLCHAIN_PREFIX := $(TOOLCHAIN_ROOT)/bin/$(TOOLCHAIN_NAME)-

TARGET_CC := $(LLVM_TOOLCHAIN_PREFIX)clang$(HOST_EXEEXT)
TARGET_CXX := $(LLVM_TOOLCHAIN_PREFIX)clang++$(HOST_EXEEXT)

#
# CFLAGS and LDFLAGS
#

TARGET_CFLAGS := \
    -gcc-toolchain $(call host-path,$(TOOLCHAIN_ROOT)) \
    -fpic \
    -ffunction-sections \
    -funwind-tables \
    -fstack-protector-strong \
    -Wno-invalid-command-line-argument \
    -Wno-unused-command-line-argument \
    -no-canonical-prefixes

# Disable integrated-as for better compatibility
TARGET_CFLAGS += \
    -fno-integrated-as

TARGET_LDFLAGS += \
    -gcc-toolchain $(call host-path,$(TOOLCHAIN_ROOT)) \
    -no-canonical-prefixes

ifneq ($(filter %armeabi-v7a,$(TARGET_ARCH_ABI)),)
    LLVM_TRIPLE := armv7-none-linux-androideabi

    TARGET_CFLAGS += -target $(LLVM_TRIPLE) \
                     -march=armv7-a \
                     -mfloat-abi=softfp \
                     -mfpu=vfpv3-d16

    TARGET_LDFLAGS += -target $(LLVM_TRIPLE) \
                      -Wl,--fix-cortex-a8

    GCCLIB_SUBDIR := armv7-a
else
ifneq ($(filter %armeabi-v7a-hard,$(TARGET_ARCH_ABI)),)
    LLVM_TRIPLE := armv7-none-linux-androideabi

    TARGET_CFLAGS += -target $(LLVM_TRIPLE) \
                     -march=armv7-a \
                     -mfpu=vfpv3-d16 \
                     -mhard-float \
                     -D_NDK_MATH_NO_SOFTFP=1

    TARGET_LDFLAGS += -target $(LLVM_TRIPLE) \
                      -Wl,--fix-cortex-a8 \
                      -Wl,--no-warn-mismatch \
                      -lm_hard

    GCCLIB_SUBDIR := armv7-a
else
    LLVM_TRIPLE := armv5te-none-linux-androideabi

    TARGET_CFLAGS += -target $(LLVM_TRIPLE) \
                     -march=armv5te \
                     -mtune=xscale \
                     -msoft-float

    TARGET_LDFLAGS += -target $(LLVM_TRIPLE)

    GCCLIB_SUBDIR :=
endif
endif

GCCLIB_ROOT := $(call get-gcclibs-path,$(NDK_ROOT),$(TOOLCHAIN_NAME))

TARGET_CFLAGS.neon := -mfpu=neon

TARGET_arm_release_CFLAGS :=  -O2 \
                              -g \
                              -DNDEBUG \
                              -fomit-frame-pointer \
                              -fstrict-aliasing

TARGET_thumb_release_CFLAGS := -mthumb \
                               -Os \
                               -g \
                               -DNDEBUG \
                               -fomit-frame-pointer \
                               -fno-strict-aliasing

# When building for debug, compile everything as arm.
TARGET_arm_debug_CFLAGS := $(TARGET_arm_release_CFLAGS) \
                           -O0 \
                           -UNDEBUG \
                           -fno-omit-frame-pointer \
                           -fno-strict-aliasing

TARGET_thumb_debug_CFLAGS := $(TARGET_thumb_release_CFLAGS) \
                             -O0 \
                             -UNDEBUG \
                             -marm \
                             -fno-omit-frame-pointer

# This function will be called to determine the target CFLAGS used to build
# a C or Assembler source file, based on its tags.
#
TARGET-process-src-files-tags = \
$(eval __arm_sources := $(call get-src-files-with-tag,arm)) \
$(eval __thumb_sources := $(call get-src-files-without-tag,arm)) \
$(eval __debug_sources := $(call get-src-files-with-tag,debug)) \
$(eval __release_sources := $(call get-src-files-without-tag,debug)) \
$(call set-src-files-target-cflags, \
    $(call set_intersection,$(__arm_sources),$(__debug_sources)), \
    $(TARGET_arm_debug_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(call set_intersection,$(__arm_sources),$(__release_sources)),\
    $(TARGET_arm_release_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(call set_intersection,$(__arm_sources),$(__debug_sources)),\
    $(TARGET_arm_debug_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(call set_intersection,$(__thumb_sources),$(__release_sources)),\
    $(TARGET_thumb_release_CFLAGS)) \
$(call set-src-files-target-cflags,\
    $(call set_intersection,$(__thumb_sources),$(__debug_sources)),\
    $(TARGET_thumb_debug_CFLAGS)) \
$(call add-src-files-target-cflags,\
    $(call get-src-files-with-tag,neon),\
    $(TARGET_CFLAGS.neon)) \
$(call set-src-files-text,$(__arm_sources),arm) \
$(call set-src-files-text,$(__thumb_sources),thumb)
