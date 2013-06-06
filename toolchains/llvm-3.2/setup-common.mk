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

TOOLCHAIN_NAME   := clang-3.2
TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREBUILT_ROOT)/bin/
LLVM_TRIPLE := le32-none-ndk

# For sources/cxx-stl/gnu-libstdc++/$(TOOLCHAIN_VERSION)/libs/*/libsupc++.a
TOOLCHAIN_VERSION := 4.7

TARGET_CC := $(TOOLCHAIN_PREFIX)clang$(HOST_EXEEXT)
TARGET_CXX := $(TOOLCHAIN_PREFIX)clang++$(HOST_EXEEXT)
TARGET_LD := $(TOOLCHAIN_PREFIX)clang++$(HOST_EXEEXT)
TARGET_AR := $(TOOLCHAIN_PREFIX)llvm-ar$(HOST_EXEEXT)
TARGET_STRIP := $(TOOLCHAIN_PREFIX)$(LLVM_TRIPLE)-strip$(HOST_EXEEXT)

# Compiler runtime is determined in bc2native
TARGET_LIBGCC :=

BC2NATIVE := $(HOST_PYTHON) $(TOOLCHAIN_PREBUILT_ROOT)/bin/ndk-bc2native.py

TARGET_CFLAGS := \
    -target $(LLVM_TRIPLE) \
    -emit-llvm \
    -ffunction-sections \
    -funwind-tables \
    -fPIC \
    -no-canonical-prefixes
# -nostdlibinc

#TARGET_CXXFLAGS := $(TARGET_CFLAGS) -fno-exceptions -fno-rtti

# reset backend flags
TARGET_NO_EXECUTE_CFLAGS :=

# Add and LDFLAGS for the target here
TARGET_LDFLAGS := \
    -target $(LLVM_TRIPLE) \
    -emit-llvm \
    -no-canonical-prefixes

TARGET_C_INCLUDES := \
    $(SYSROOT_INC)/usr/include

TARGET_release_CFLAGS := -O2 \
                         -g \
                         -DNDEBUG \
                         -fomit-frame-pointer \
                         -fstrict-aliasing

TARGET_debug_CFLAGS := $(TARGET_release_CFLAGS) \
                       -O0 \
                       -UNDEBUG \
                       -fno-omit-frame-pointer \
                       -fno-strict-aliasing

# This function will be called to determine the target CFLAGS used to build
# a C or Assembler source file, based on its tags.
#
TARGET-process-src-files-tags = \
$(eval __debug_sources := $(call get-src-files-with-tag,debug)) \
$(eval __release_sources := $(call get-src-files-without-tag,debug)) \
$(call set-src-files-target-cflags, $(__debug_sources), $(TARGET_debug_CFLAGS)) \
$(call set-src-files-target-cflags, $(__release_sources),$(TARGET_release_CFLAGS)) \
$(call set-src-files-text,$(LOCAL_SRC_FILES),bc$(space)$(space)) \

ifeq ($(strip $(filter-out $(NDK_KNOWN_ABIS),$(TARGET_ARCH_ABI))),)

define cmd-build-shared-library
$(PRIVATE_CXX) \
    -Wl,-soname,$(notdir $(LOCAL_BUILT_MODULE)) \
    -shared \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT_LINK)) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$(LOCAL_BUILT_MODULE)) && \
    $(BC2NATIVE) \
    --ndk-dir=$(NDK_ROOT) \
    --abi=$(TARGET_ARCH_ABI) \
    --platform=$(TARGET_PLATFORM) \
    --file $(call host-path, $(LOCAL_BUILT_MODULE)) $(patsubst %.bc,%.so,$(call host-path,$(LOCAL_BUILT_MODULE)))
endef

define cmd-build-executable
$(PRIVATE_CXX) \
    -Wl,--gc-sections \
    -Wl,-z,nocopyreloc \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT_LINK)) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$(LOCAL_BUILT_MODULE)) && \
    $(BC2NATIVE) \
    --ndk-dir=$(NDK_ROOT) \
    --abi=$(TARGET_ARCH_ABI) \
    --platform=$(TARGET_PLATFORM) \
    --file $(call host-path,$(LOCAL_BUILT_MODULE)) $(call host-path,$(LOCAL_BUILT_MODULE))
endef

endif
