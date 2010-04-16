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

# this file is included repeatedly from build/core/setup-abi.mk and is used
# to setup the target toolchain for a given platform/abi combination.
#

$(call assert-defined,TARGET_PLATFORM TARGET_ARCH TARGET_ARCH_ABI)
$(call assert-defined,NDK_APPS)

# Check that we have a toolchain that supports the current ABI.
# NOTE: If NDK_TOOLCHAIN is defined, we're going to use it.
#
ifndef NDK_TOOLCHAIN
    TARGET_TOOLCHAIN_LIST := $(strip $(sort $(NDK_ABI.$(TARGET_ARCH_ABI).toolchains)))
    ifndef TARGET_TOOLCHAIN_LIST
        $(call __ndk_info,There is no toolchain that supports the $(TARGET_ARCH_ABI) ABI.)
        $(call __ndk_info,Please modify the APP_ABI definition in $(NDK_APP_APPLICATION_MK) to use)
        $(call __ndk_info,a set of the following values: $(NDK_ALL_ABIS))
        $(call __ndk_error,Aborting)
    endif
    # Select the last toolchain from the sorted list.
    # For now, this is enough to select armeabi-4.4.0 by default for ARM
    TARGET_TOOLCHAIN := $(lastword $(TARGET_TOOLCHAIN_LIST))
    $(call ndk_log,Using target toolchain '$(TARGET_TOOLCHAIN)' for '$(TARGET_ARCH_ABI)' ABI)
else # NDK_TOOLCHAIN is not empty
    TARGET_TOOLCHAIN_LIST := $(strip $(filter $(NDK_TOOLCHAIN),$(NDK_ABI.$(TARGET_ARCH_ABI).toolchains)))
    ifndef TARGET_TOOLCHAIN_LIST
        $(call __ndk_info,The selected toolchain ($(NDK_TOOLCHAIN)) does not support the $(TARGET_ARCH_ABI) ABI.)
        $(call __ndk_info,Please modify the APP_ABI definition in $(NDK_APP_APPLICATION_MK) to use)
        $(call __ndk_info,a set of the following values: $(NDK_TOOLCHAIN.$(NDK_TOOLCHAIN).abis))
        $(call __ndk_info,Or change your NDK_TOOLCHAIN definition.)
        $(call __ndk_error,Aborting)
    endif
    TARGET_TOOLCHAIN := $(NDK_TOOLCHAIN)
endif # NDK_TOOLCHAIN is not empty

TARGET_ABI := $(TARGET_PLATFORM)-$(TARGET_ARCH_ABI)

# setup sysroot-related variables. The SYSROOT point to a directory
# that contains all public header files for a given platform, plus
# some libraries and object files used for linking the generated
# target files properly.
#
SYSROOT := $(NDK_ROOT)/build/platforms/$(TARGET_PLATFORM)/arch-$(TARGET_ARCH)

TARGET_CRTBEGIN_STATIC_O  := $(SYSROOT)/usr/lib/crtbegin_static.o
TARGET_CRTBEGIN_DYNAMIC_O := $(SYSROOT)/usr/lib/crtbegin_dynamic.o
TARGET_CRTEND_O           := $(SYSROOT)/usr/lib/crtend_android.o

TARGET_PREBUILT_SHARED_LIBRARIES := libc libstdc++ libm
TARGET_PREBUILT_SHARED_LIBRARIES := $(TARGET_PREBUILT_SHARED_LIBRARIES:%=$(SYSROOT)/usr/lib/%.so)

# now call the toolchain-specific setup script
include $(NDK_TOOLCHAIN.$(TARGET_TOOLCHAIN).setup)

# We expect the gdbserver binary for this toolchain to be located at the same
# place than the target C compiler.
TARGET_GDBSERVER := $(dir $(TARGET_CC))/gdbserver

# compute NDK_APP_DST_DIR as the destination directory for the generated files
NDK_APP_DST_DIR := $(NDK_APP_PROJECT_PATH)/libs/$(TARGET_ARCH_ABI)

# Ensure that for debuggable applications, gdbserver will be copied to
# the proper location
ifeq ($(NDK_APP_DEBUGGABLE),true)

NDK_APP_GDBSERVER := $(NDK_APP_DST_DIR)/gdbserver

installed_modules: $(NDK_APP_GDBSERVER)

$(NDK_APP_GDBSERVER): PRIVATE_NAME    := $(TOOLCHAIN_NAME)
$(NDK_APP_GDBSERVER): PRIVATE_SRC     := $(TARGET_GDBSERVER)
$(NDK_APP_GDBSERVER): PRIVATE_DST_DIR := $(NDK_APP_DST_DIR)
$(NDK_APP_GDBSERVER): PRIVATE_DST     := $(NDK_APP_GDBSERVER)

$(NDK_APP_GDBSERVER): clean-installed-binaries
	@ echo "Gdbserver      : [$(PRIVATE_NAME)] $(PRIVATE_DST)"
	$(hide) mkdir -p $(PRIVATE_DST_DIR)
	$(hide) install -p $(PRIVATE_SRC) $(PRIVATE_DST)

NDK_APP_GDBSETUP := $(NDK_APP_DST_DIR)/gdb.setup
installed_modules: $(NDK_APP_GDBSETUP)

$(NDK_APP_GDBSETUP)::
	@ echo "Gdbsetup       : $(PRIVATE_DST)"
	$(hide) echo "set solib-search-path $(PRIVATE_SOLIB_PATH)" > $(PRIVATE_DST)
	$(hide) echo "directory $(SYSROOT)/usr/include" >> $(PRIVATE_DST)

$(NDK_APP_GDBSETUP):: PRIVATE_DST := $(NDK_APP_GDBSETUP)
$(NDK_APP_GDBSETUP):: PRIVATE_SOLIB_PATH := $(TARGET_OUT)
$(NDK_APP_GDBSETUP):: PRIVATE_SRC_DIRS := $(SYSROOT)/usr/include

endif

clean-installed-binaries::

# free the dictionary of LOCAL_MODULE definitions
$(call modules-clear)

# now parse the Android.mk for the application
include $(NDK_APP_BUILD_SCRIPT)

# Now compute the closure of all module dependencies.
#
# If APP_MODULES is not defined in the Application.mk, we
# will build all modules that were listed from the top-level Android.mk
#
ifeq ($(strip $(NDK_APP_MODULES)),)
    WANTED_MODULES := $(call modules-get-list)
else
    WANTED_MODULES := $(call module-get-all-dependencies,$(NDK_APP_MODULES))
endif

WANTED_INSTALLED_MODULES += $(call map,module-get-installed,$(WANTED_MODULES))
