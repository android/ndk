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

# this file is used to prepare the NDK to build with the arm-eabi-4.4.0
# toolchain any number of source files
#
# its purpose is to define (or re-define) templates used to build
# various sources into target object files, libraries or executables.
#
# Note that this file may end up being parsed several times in future
# revisions of the NDK.
#

TARGET_CFLAGS := \
    -fpic \
    -ffunction-sections \
    -funwind-tables \
    -fstack-protector \
    -D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ \
    -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ \

TARGET_LDFLAGS :=

TARGET_C_INCLUDES := \
    $(SYSROOT)/usr/include

# This is to avoid the dreaded warning compiler message:
#   note: the mangling of 'va_list' has changed in GCC 4.4
#
# The fact that the mangling changed does not affect the NDK ABI
# very fortunately (since none of the exposed APIs used va_list
# in their exported C++ functions). Also, GCC 4.5 has already
# removed the warning from the compiler.
#
TARGET_CFLAGS += -Wno-psabi

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    TARGET_CFLAGS += -march=armv7-a \
                     -mfloat-abi=softfp \
                     -mfpu=vfp

    TARGET_LDFLAGS += -Wl,--fix-cortex-a8
else
    TARGET_CFLAGS += -march=armv5te \
                            -mtune=xscale \
                            -msoft-float
endif

TARGET_CFLAGS.neon := -mfpu=neon

TARGET_arm_release_CFLAGS :=  -O2 \
                              -fomit-frame-pointer \
                              -fstrict-aliasing    \
                              -funswitch-loops     \
                              -finline-limit=300

TARGET_thumb_release_CFLAGS := -mthumb \
                               -Os \
                               -fomit-frame-pointer \
                               -fno-strict-aliasing \
                               -finline-limit=64

# When building for debug, compile everything as arm.
TARGET_arm_debug_CFLAGS := $(TARGET_arm_release_CFLAGS) \
                           -fno-omit-frame-pointer \
                           -fno-strict-aliasing

TARGET_thumb_debug_CFLAGS := $(TARGET_thumb_release_CFLAGS) \
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
$(call set-src-files-text,$(__arm_sources),arm$(space)$(space)) \
$(call set-src-files-text,$(__thumb_sources),thumb)

#
# We need to add -lsupc++ to the final link command to make exceptions
# and RTTI work properly (when -fexceptions and -frtti are used).
#
# Normally, the toolchain should be configured to do that automatically,
# this will be debugged later.
#
define cmd-build-shared-library
$(PRIVATE_CXX) \
    -Wl,-soname,$(notdir $@) \
    -shared \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT)) \
    $(call host-path, $(PRIVATE_OBJECTS)) \
    $(call link-whole-archives,$(PRIVATE_WHOLE_STATIC_LIBRARIES)) \
    $(call host-path,\
        $(PRIVATE_STATIC_LIBRARIES) \
        $(PRIVATE_LIBGCC) \
        $(PRIVATE_SHARED_LIBRARIES)) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$@)
endef

define cmd-build-executable
$(PRIVATE_CXX) \
    -Wl,--gc-sections \
    -Wl,-z,nocopyreloc \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT)) \
    $(call host-path, $(PRIVATE_OBJECTS)) \
    $(call link-whole-archives,$(PRIVATE_WHOLE_STATIC_LIBRARIES)) \
    $(call host-path,\
        $(PRIVATE_STATIC_LIBRARIES) \
        $(PRIVATE_LIBGCC) \
        $(PRIVATE_SHARED_LIBRARIES)) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$@)
endef
