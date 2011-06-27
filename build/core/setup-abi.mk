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

# this file is included multiple times by build/core/setup-app.mk
#

$(call ndk_log,Building application '$(NDK_APP_NAME)' for ABI '$(TARGET_ARCH_ABI)')

# Map ABIs to a target architecture
TARGET_ARCH_for_armeabi     := arm
TARGET_ARCH_for_armeabi-v7a := arm
TARGET_ARCH_for_x86         := x86

TARGET_ARCH := $(TARGET_ARCH_for_$(TARGET_ARCH_ABI))

TARGET_OUT  := $(NDK_APP_OUT)/$(_app)/$(TARGET_ARCH_ABI)

# Special handling for x86: The minimal platform is android-9 here
# For now, handle this with a simple substitution. We may want to implement
# more general filtering in the future when introducing other ABIs.
TARGET_PLATFORM_SAVED := $(TARGET_PLATFORM)
ifeq ($(TARGET_ARCH),x86)
$(foreach _plat,3 4 5 8,\
    $(eval TARGET_PLATFORM := $$(subst android-$(_plat),android-9,$$(TARGET_PLATFORM)))\
)
endif

# Separate the debug and release objects. This prevents rebuilding
# everything when you switch between these two modes. For projects
# with lots of C++ sources, this can be a considerable time saver.
ifeq ($(NDK_APP_OPTIM),debug)
TARGET_OBJS := $(TARGET_OUT)/objs-debug
else
TARGET_OBJS := $(TARGET_OUT)/objs
endif

TARGET_GDB_SETUP := $(TARGET_OUT)/setup.gdb

include $(BUILD_SYSTEM)/setup-toolchain.mk

# Restore TARGET_PLATFORM, see above.
TARGET_PLATFORM := $(TARGET_PLATFORM_SAVED)
