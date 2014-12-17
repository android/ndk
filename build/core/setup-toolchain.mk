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
$(call assert-defined,NDK_APPS NDK_APP_STL)

LLVM_VERSION_LIST := 2.6 2.7 2.8 2.9 3.0 3.1 3.2 3.3 3.4 3.5
NDK_64BIT_TOOLCHAIN_LIST := clang3.5 clang3.4 4.9

# Check that we have a toolchain that supports the current ABI.
# NOTE: If NDK_TOOLCHAIN is defined, we're going to use it.
#
ifndef NDK_TOOLCHAIN
    TARGET_TOOLCHAIN_LIST := $(strip $(sort $(NDK_ABI.$(TARGET_ARCH_ABI).toolchains)))

    # Filter out the Clang toolchain, so that we can keep GCC as the default
    # toolchain.
    $(foreach _ver,$(LLVM_VERSION_LIST), \
        $(eval TARGET_TOOLCHAIN_LIST := \
            $(filter-out %-clang$(_ver),$(TARGET_TOOLCHAIN_LIST))))

    ifeq (,$(findstring 64,$(TARGET_ARCH_ABI)))
      # Filter out 4.6 and 4.7 which are deprecated
      __filtered_toolchain_list := $(filter-out %4.6 %4.7,$(TARGET_TOOLCHAIN_LIST))
      ifdef __filtered_toolchain_list
          TARGET_TOOLCHAIN_LIST := $(__filtered_toolchain_list)
      endif
    else
      # Filter out 4.6, 4.7 and 4.8 which don't have good 64-bit support in all supported arch
      TARGET_TOOLCHAIN_LIST := $(filter-out %4.6 %4.7 %4.8 %4.8l,$(TARGET_TOOLCHAIN_LIST))
    endif

    ifndef TARGET_TOOLCHAIN_LIST
        $(call __ndk_info,There is no toolchain that supports the $(TARGET_ARCH_ABI) ABI.)
        $(call __ndk_info,Please modify the APP_ABI definition in $(NDK_APP_APPLICATION_MK) to use)
        $(call __ndk_info,a set of the following values: $(NDK_ALL_ABIS))
        $(call __ndk_error,Aborting)
    endif
    # Select the last toolchain from the sorted list.
    # For now, this is enough to select by default gcc4.8 for 32-bit, and 4.9 for 64-bit, the the
    # latest llvm if no gcc
    ifneq (,$(filter-out llvm-%,$(TARGET_TOOLCHAIN_LIST)))
        TARGET_TOOLCHAIN := $(firstword $(TARGET_TOOLCHAIN_LIST))
    else
        TARGET_TOOLCHAIN := $(lastword $(TARGET_TOOLCHAIN_LIST))
    endif
    # If NDK_TOOLCHAIN_VERSION is defined, we replace the toolchain version
    # suffix with it.
    #
    ifdef NDK_TOOLCHAIN_VERSION
        # Replace "clang" with the most recent verion
        ifeq ($(NDK_TOOLCHAIN_VERSION),clang)
            override NDK_TOOLCHAIN_VERSION := clang$(lastword $(LLVM_VERSION_LIST))
        endif
        __use_ndk_toolchain_version := true
        ifneq (,$(findstring 64,$(TARGET_ARCH_ABI)))
            # don't allow NDK_TOOLCHAIN_VERSION to change if it doesn't support 64-bit
            ifeq (,$(filter $(NDK_64BIT_TOOLCHAIN_LIST),$(NDK_TOOLCHAIN_VERSION)))
                $(call ndk_log,Specified NDK_TOOLCHAIN_VERSION $(NDK_TOOLCHAIN_VERSION) does not support 64-bit)
                $(call ndk_log,Using default target toolchain '$(TARGET_TOOLCHAIN)' for '$(TARGET_ARCH_ABI)' ABI)
                __use_ndk_toolchain_version := false;
            endif
        endif
        ifeq ($(__use_ndk_toolchain_version),true)
            # We assume the toolchain name uses dashes (-) as separators and doesn't
            # contain any space. The following is a bit subtle, but essentially
            # does the following:
            #
            #   1/ Use 'subst' to convert dashes into spaces, this generates a list
            #   2/ Use 'chop' to remove the last element of the list
            #   3/ Use 'subst' again to convert the spaces back into dashes
            #
            # So it TARGET_TOOLCHAIN is 'foo-bar-zoo-xxx', then
            # TARGET_TOOLCHAIN_BASE will be 'foo-bar-zoo'
            #
            TARGET_TOOLCHAIN_BASE := $(subst $(space),-,$(call chop,$(subst -,$(space),$(TARGET_TOOLCHAIN))))
            # if TARGET_TOOLCHAIN_BASE is llvm, remove clang from NDK_TOOLCHAIN_VERSION
            VERSION := $(NDK_TOOLCHAIN_VERSION)
            ifeq ($(TARGET_TOOLCHAIN_BASE),llvm)
                VERSION := $(subst clang,,$(NDK_TOOLCHAIN_VERSION))
            endif
            TARGET_TOOLCHAIN := $(TARGET_TOOLCHAIN_BASE)-$(VERSION)
            $(call ndk_log,Using target toolchain '$(TARGET_TOOLCHAIN)' for '$(TARGET_ARCH_ABI)' ABI (through NDK_TOOLCHAIN_VERSION))
        endif
    else
        $(call ndk_log,Using target toolchain '$(TARGET_TOOLCHAIN)' for '$(TARGET_ARCH_ABI)' ABI)
    endif
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

# setup sysroot variable.
# SYSROOT_INC points to a directory that contains all public header
# files for a given platform, and
# SYSROOT_LIB points to libraries and object files used for linking
# the generated target files properly.
#
SYSROOT_INC := $(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-$(TARGET_ARCH)
SYSROOT_LINK := $(SYSROOT_INC)

TARGET_PREBUILT_SHARED_LIBRARIES :=

# Define default values for TOOLCHAIN_NAME, this can be overriden in
# the setup file.
TOOLCHAIN_NAME   := $(TARGET_TOOLCHAIN)
TOOLCHAIN_VERSION := $(call last,$(subst -,$(space),$(TARGET_TOOLCHAIN)))

# Define the root path of the toolchain in the NDK tree.
TOOLCHAIN_ROOT   := $(NDK_ROOT)/toolchains/$(TOOLCHAIN_NAME)

# Define the root path where toolchain prebuilts are stored
TOOLCHAIN_PREBUILT_ROOT := $(call host-prebuilt-tag,$(TOOLCHAIN_ROOT))

# Do the same for TOOLCHAIN_PREFIX. Note that we must chop the version
# number from the toolchain name, e.g. arm-eabi-4.4.0 -> path/bin/arm-eabi-
# to do that, we split at dashes, remove the last element, then merge the
# result. Finally, add the complete path prefix.
#
TOOLCHAIN_PREFIX := $(call merge,-,$(call chop,$(call split,-,$(TOOLCHAIN_NAME))))-
TOOLCHAIN_PREFIX := $(TOOLCHAIN_PREBUILT_ROOT)/bin/$(TOOLCHAIN_PREFIX)

# We expect the gdbserver binary for this toolchain to be located at its root.
TARGET_GDBSERVER := $(NDK_ROOT)/prebuilt/android-$(TARGET_ARCH)/gdbserver/gdbserver

# compute NDK_APP_DST_DIR as the destination directory for the generated files
NDK_APP_DST_DIR := $(NDK_APP_LIBS_OUT)/$(TARGET_ARCH_ABI)
# install armeabi-v7a-hard to lib/armeabi-v7a, unless under testing where env. var. _NDK_TESTING_ALL_
# is set to one of yes, all, all32, or all64
ifeq (,$(filter yes all all32 all64,$(_NDK_TESTING_ALL_)))
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
NDK_APP_DST_DIR := $(NDK_APP_LIBS_OUT)/armeabi-v7a
endif
endif

# Default build commands, can be overriden by the toolchain's setup script
include $(BUILD_SYSTEM)/default-build-commands.mk

# now call the toolchain-specific setup script
include $(NDK_TOOLCHAIN.$(TARGET_TOOLCHAIN).setup)

clean-installed-binaries::

# Ensure that for debuggable applications, gdbserver will be copied to
# the proper location

NDK_APP_GDBSERVER := $(NDK_APP_DST_DIR)/gdbserver
NDK_APP_GDBSETUP := $(NDK_APP_DST_DIR)/gdb.setup

ifeq ($(NDK_APP_DEBUGGABLE),true)
ifeq ($(TARGET_SONAME_EXTENSION),.so)

installed_modules: $(NDK_APP_GDBSERVER)

$(NDK_APP_GDBSERVER): PRIVATE_ABI     := $(TARGET_ARCH_ABI)
$(NDK_APP_GDBSERVER): PRIVATE_NAME    := $(TOOLCHAIN_NAME)
$(NDK_APP_GDBSERVER): PRIVATE_SRC     := $(TARGET_GDBSERVER)
$(NDK_APP_GDBSERVER): PRIVATE_DST     := $(NDK_APP_GDBSERVER)

$(call generate-file-dir,$(NDK_APP_GDBSERVER))

$(NDK_APP_GDBSERVER): clean-installed-binaries
	$(call host-echo-build-step,$(PRIVATE_ABI),Gdbserver) "[$(PRIVATE_NAME)] $(call pretty-dir,$(PRIVATE_DST))"
	$(hide) $(call host-install,$(PRIVATE_SRC),$(PRIVATE_DST))
endif

# Install gdb.setup for both .so and .bc projects
ifneq (,$(filter $(TARGET_SONAME_EXTENSION),.so .bc))
installed_modules: $(NDK_APP_GDBSETUP)

$(NDK_APP_GDBSETUP): PRIVATE_ABI := $(TARGET_ARCH_ABI)
$(NDK_APP_GDBSETUP): PRIVATE_DST := $(NDK_APP_GDBSETUP)
$(NDK_APP_GDBSETUP): PRIVATE_SOLIB_PATH := $(TARGET_OUT)
$(NDK_APP_GDBSETUP): PRIVATE_SRC_DIRS := $(SYSROOT_INC)/usr/include

$(NDK_APP_GDBSETUP):
	$(call host-echo-build-step,$(PRIVATE_ABI),Gdbsetup) "$(call pretty-dir,$(PRIVATE_DST))"
	$(hide) $(HOST_ECHO) "set solib-search-path $(call host-path,$(PRIVATE_SOLIB_PATH))" > $(PRIVATE_DST)
	$(hide) $(HOST_ECHO) "directory $(call host-path,$(call remove-duplicates,$(PRIVATE_SRC_DIRS)))" >> $(PRIVATE_DST)

$(call generate-file-dir,$(NDK_APP_GDBSETUP))

# This prevents parallel execution to clear gdb.setup after it has been written to
$(NDK_APP_GDBSETUP): clean-installed-binaries
endif
endif

# free the dictionary of LOCAL_MODULE definitions
$(call modules-clear)

$(call ndk-stl-select,$(NDK_APP_STL))

# now parse the Android.mk for the application, this records all
# module declarations, but does not populate the dependency graph yet.
include $(NDK_APP_BUILD_SCRIPT)

$(call ndk-stl-add-dependencies,$(NDK_APP_STL))

# recompute all dependencies between modules
$(call modules-compute-dependencies)

# for debugging purpose
ifdef NDK_DEBUG_MODULES
$(call modules-dump-database)
endif

# now, really build the modules, the second pass allows one to deal
# with exported values
$(foreach __pass2_module,$(__ndk_modules),\
    $(eval LOCAL_MODULE := $(__pass2_module))\
    $(eval include $(BUILD_SYSTEM)/build-binary.mk)\
)

# Now compute the closure of all module dependencies.
#
# If APP_MODULES is not defined in the Application.mk, we
# will build all modules that were listed from the top-level Android.mk
# and the installable imported ones they depend on
#
ifeq ($(strip $(NDK_APP_MODULES)),)
    WANTED_MODULES := $(call modules-get-all-installable,$(modules-get-top-list))
    ifeq (,$(strip $(WANTED_MODULES)))
        WANTED_MODULES := $(modules-get-top-list)
        $(call ndk_log,[$(TARGET_ARCH_ABI)] No installable modules in project - forcing static library build)
    endif
else
    WANTED_MODULES := $(call module-get-all-dependencies,$(NDK_APP_MODULES))
endif

$(call ndk_log,[$(TARGET_ARCH_ABI)] Modules to build: $(WANTED_MODULES))

WANTED_INSTALLED_MODULES += $(call map,module-get-installed,$(WANTED_MODULES))
