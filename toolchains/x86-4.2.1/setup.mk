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

# this file is used to prepare the NDK to build with the x86-4.2.1
# toolchain any number of source files
#
# its purpose is to define (or re-define) templates used to build
# various sources into target object files, libraries or executables.
#
# Note that this file may end up being parsed several times in future
# revisions of the NDK.
#

TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREBUILT_ROOT)/bin/i686-android-linux-gnu-

TARGET_CFLAGS := \
    -msoft-float -fpic \
    -ffunction-sections \
    -funwind-tables \
    -fstack-protector \
    -fno-short-enums

#
# Normally, this macro should be defined by the toolchain automatically.
# Unfortunately, this is not the case, so add it manually. Note that
# the arm-linux-androideabi toolchain does not have this problem.
#
TARGET_CFLAGS += -D__ANDROID__

TARGET_LDFLAGS :=

TARGET_C_INCLUDES := \
    $(SYSROOT)/usr/include

TARGET_x86_release_CFLAGS :=  -O2 \
                              -fomit-frame-pointer \
                              -fstrict-aliasing    \
                              -funswitch-loops     \
                              -finline-limit=300

# When building for debug, compile everything as x86.
TARGET_x86_debug_CFLAGS := $(TARGET_x86_release_CFLAGS) \
                           -fno-omit-frame-pointer \
                           -fno-strict-aliasing

# This function will be called to determine the target CFLAGS used to build
# a C or Assembler source file, based on its tags.
#
TARGET-process-src-files-tags = \
$(eval __debug_sources := $(call get-src-files-with-tag,debug)) \
$(eval __release_sources := $(call get-sr-files-without-tag,debug)) \
$(call set-src-files-target-cflags, $(__debug_sources), $(TARGET_x86_debug_CFLAGS)) \
$(call set-src-files-target-cflags, $(__release_sources),$(TARGET_x86_release_CFLAGS)) \
$(call set-src-files-text,$(LOCAL_SRC_FILES),x86$(space)$(space)) \

# The ABI-specific sub-directory that the SDK tools recognize for
# this toolchain's generated binaries
TARGET_ABI_SUBDIR := x86
