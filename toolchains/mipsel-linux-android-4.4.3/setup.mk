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

# this file is used to prepare the NDK to build with the mipsel 4.4.3
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
        -fno-strict-aliasing \
        -finline-functions \
        -ffunction-sections \
        -funwind-tables \
        -fmessage-length=0 \
        -fno-inline-functions-called-once \
        -fgcse-after-reload \
        -frerun-cse-after-loop \
        -frename-registers \

TARGET_LDFLAGS :=
TARGET_LDSCRIPT_XSC := -Wl,-T,$(call host-path,$(TOOLCHAIN_ROOT))/mipself.xsc
TARGET_LDSCRIPT_X := -Wl,-T,$(call host-path,$(TOOLCHAIN_ROOT))/mipself.x

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

TARGET_mips_release_CFLAGS :=  -O2 \
                               -fomit-frame-pointer \
                               -funswitch-loops     \
                               -finline-limit=300

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

#
# We need to add -lsupc++ to the final link command to make exceptions
# and RTTI work properly (when -fexceptions and -frtti are used).
#
# Normally, the toolchain should be configured to do that automatically,
# this will be debugged later.
#
define cmd-build-shared-library
$(PRIVATE_CXX) \
    $(PRIVATE_LDSCRIPT_XSC) \
    -Wl,-soname,$(notdir $@) \
    -shared \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT)) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$@)
endef

define cmd-build-executable
$(PRIVATE_CXX) \
    $(PRIVATE_LDSCRIPT_X) \
    -Wl,--gc-sections \
    -Wl,-z,nocopyreloc \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT)) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$@)
endef
