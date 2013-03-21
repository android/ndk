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

# this file is used to prepare the NDK to build with the clang-3.1
# toolchain any number of source files
#
# its purpose is to define (or re-define) templates used to build
# various sources into target object files, libraries or executables.
#
# Note that this file may end up being parsed several times in future
# revisions of the NDK.
#

ifneq ($(filter %armeabi-v7a,$(TARGET_ARCH_ABI)),)

SYSROOT_LINK     := $(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm
TARGET_GDBSERVER := $(NDK_ROOT)/prebuilt/android-arm/gdbserver/gdbserver
TARGET_ARCH_ABI  := armeabi-v7a
NDK_APP_DST_DIR  := $(NDK_APP_PROJECT_PATH)/libs/$(TARGET_ARCH_ABI)
TARGET_LDLIBS    := $(NDK_ROOT)/sources/android/libportable/libs/armeabi-v7a/libportable.a $(TARGET_LDLIBS)
TARGET_LDFLAGS   += -Wl,@$(NDK_ROOT)/sources/android/libportable/libs/armeabi-v7a/libportable.wrap
include $(NDK_ROOT)/toolchains/arm-linux-androideabi-clang3.1/setup.mk

else
ifneq ($(filter %armeabi,$(TARGET_ARCH_ABI)),)

SYSROOT_LINK     := $(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm
TARGET_GDBSERVER := $(NDK_ROOT)/prebuilt/android-arm/gdbserver/gdbserver
TARGET_ARCH_ABI  := armeabi
NDK_APP_DST_DIR  := $(NDK_APP_PROJECT_PATH)/libs/$(TARGET_ARCH_ABI)
TARGET_LDLIBS    := $(NDK_ROOT)/sources/android/libportable/libs/armeabi/libportable.a $(TARGET_LDLIBS)
TARGET_LDFLAGS   += -Wl,@$(NDK_ROOT)/sources/android/libportable/libs/armeabi/libportable.wrap
include $(NDK_ROOT)/toolchains/arm-linux-androideabi-clang3.1/setup.mk

else
ifneq ($(filter %x86,$(TARGET_ARCH_ABI)),)

SYSROOT_LINK     := $(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-x86
TARGET_GDBSERVER := $(NDK_ROOT)/prebuilt/android-x86/gdbserver/gdbserver
TARGET_ARCH_ABI  := x86
NDK_APP_DST_DIR  := $(NDK_APP_PROJECT_PATH)/libs/$(TARGET_ARCH_ABI)
TARGET_LDLIBS    := $(NDK_ROOT)/sources/android/libportable/libs/x86/libportable.a $(TARGET_LDLIBS)
TARGET_LDFLAGS   += -Wl,@$(NDK_ROOT)/sources/android/libportable/libs/x86/libportable.wrap
include $(NDK_ROOT)/toolchains/x86-clang3.1/setup.mk

else
ifneq ($(filter %mips,$(TARGET_ARCH_ABI)),)

SYSROOT_LINK     := $(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-mips
TARGET_GDBSERVER := $(NDK_ROOT)/prebuilt/android-mips/gdbserver/gdbserver
TARGET_ARCH_ABI  := mips
NDK_APP_DST_DIR  := $(NDK_APP_PROJECT_PATH)/libs/$(TARGET_ARCH_ABI)
TARGET_LDLIBS    := $(NDK_ROOT)/sources/android/libportable/libs/mips/libportable.a $(TARGET_LDLIBS)
TARGET_LDFLAGS   += -Wl,@$(NDK_ROOT)/sources/android/libportable/libs/mips/libportable.wrap
include $(NDK_ROOT)/toolchains/mipsel-linux-android-clang3.1/setup.mk

else

TOOLCHAIN_NAME   := clang-3.1
TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREBUILT_ROOT)/bin/
LLVM_TRIPLE := le32-none-ndk

TARGET_CC := $(TOOLCHAIN_PREFIX)clang$(HOST_EXEEXT)
TARGET_CXX := $(TOOLCHAIN_PREFIX)clang++$(HOST_EXEEXT)
TARGET_LD := $(TOOLCHAIN_PREFIX)clang++$(HOST_EXEEXT)
TARGET_AR := $(TOOLCHAIN_PREFIX)llvm-ar$(HOST_EXEEXT)
TARGET_STRIP := $(TOOLCHAIN_PREFIX)$(LLVM_TRIPLE)-strip$(HOST_EXEEXT)

TARGET_OBJ_EXTENSION := .bc
TARGET_LIB_EXTENSION := .a #.bc
TARGET_SONAME_EXTENSION := .bc

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
$(call set-src-files-text,$(LOCAL_SRC_FILES),plus$(space)$(space)) \

endif
endif
endif
endif
