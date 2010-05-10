# Copyright (C) 2009-2010 The Android Open Source Project
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

# Initialization of the NDK build system. This file is included by
# several build scripts.
#

# Define NDK_LOG in your environment to display log traces when
# using the build scripts. See also the definition of ndk_log below.
#
NDK_LOG := $(strip $(NDK_LOG))

# Check that we have at least GNU Make 3.81
# We do this by detecting whether 'lastword' is supported
#
MAKE_TEST := $(lastword a b c d e f)
ifneq ($(MAKE_TEST),f)
    $(error Android NDK: GNU Make version $(MAKE_VERSION) is too low (should be >= 3.81)
endif
ifdef NDK_LOG
    $(info Android NDK: GNU Make version $(MAKE_VERSION) detected)
endif

# NDK_ROOT *must* be defined and point to the root of the NDK installation
NDK_ROOT := $(strip $(NDK_ROOT))
ifndef NDK_ROOT
    $(error ERROR while including init.mk: NDK_ROOT must be defined !)
endif
ifneq ($(words $(NDK_ROOT)),1)
    $(info,The Android NDK installation path contains spaces: '$(NDK_ROOT)')
    $(error,Please fix the problem by reinstalling to a different location.)
endif

# ====================================================================
#
# Define a few useful variables and functions.
# More stuff will follow in definitions.mk.
#
# ====================================================================

# Used to output warnings and error from the library, it's possible to
# disable any warnings or errors by overriding these definitions
# manually or by setting NDK_NO_WARNINGS or NDK_NO_ERRORS

__ndk_name    := Android NDK
__ndk_info     = $(info $(__ndk_name): $1 $2 $3 $4 $5)
__ndk_warning  = $(warning $(__ndk_name): $1 $2 $3 $4 $5)
__ndk_error    = $(error $(__ndk_name): $1 $2 $3 $4 $5)

ifdef NDK_NO_WARNINGS
__ndk_warning :=
endif
ifdef NDK_NO_ERRORS
__ndk_error :=
endif

# -----------------------------------------------------------------------------
# Function : ndk_log
# Arguments: 1: text to print when NDK_LOG is defined
# Returns  : None
# Usage    : $(call ndk_log,<some text>)
# -----------------------------------------------------------------------------
ifdef NDK_LOG
ndk_log = $(info $(__ndk_name): $1)
else
ndk_log :=
endif

# ====================================================================
#
# Host system auto-detection.
#
# ====================================================================

#
# Determine host system and architecture from the environment
#
HOST_OS := $(strip $(HOST_OS))
ifndef HOST_OS
    # On all modern variants of Windows (including Cygwin and Wine)
    # the OS environment variable is defined to 'Windows_NT'
    #
    # The value of PROCESSOR_ARCHITECTURE will be x86 or AMD64
    #
    ifeq ($(OS),Windows_NT)
        HOST_OS := windows
    else
        # For other systems, use the `uname` output
        UNAME := $(shell uname -s)
        ifneq (,$(findstring Linux,$(UNAME)))
            HOST_OS := linux
        endif
        ifneq (,$(findstring Darwin,$(UNAME)))
            HOST_OS := darwin
        endif
        # We should not be there, but just in case !
        ifneq (,$(findstring CYGWIN,$(UNAME)))
            HOST_OS := windows
        endif
        ifeq ($(HOST_OS),)
            $(call __ndk_info,Unable to determine HOST_OS from uname -s: $(UNAME))
            $(call __ndk_info,Please define HOST_OS in your environment.)
            $(call __ndk_error,Aborting.)
        endif
    endif
    $(call ndk_log,Host OS was auto-detected: $(HOST_OS))
else
    $(call ndk_log,Host OS from environment: $(HOST_OS))
endif

HOST_ARCH := $(strip $(HOST_ARCH))
ifndef HOST_ARCH
    ifeq ($(HOST_OS),windows)
        HOST_ARCH := $(PROCESSOR_ARCHITECTURE)
        ifeq ($(HOST_ARCH),AMD64)
            HOST_ARCH := x86
        endif
    else # HOST_OS != windows
        UNAME := $(shell uname -m)
        ifneq (,$(findstring 86,$(UNAME)))
            HOST_ARCH := x86
        endif
        # We should probably should not care at all
        ifneq (,$(findstring Power,$(UNAME)))
            HOST_ARCH := ppc
        endif
        ifeq ($(HOST_ARCH),)
            $(call __ndk_info,Unsupported host architecture: $(UNAME))
            $(call __ndk_error,Aborting)
        endif
    endif # HOST_OS != windows
    $(call ndk_log,Host CPU was auto-detected: $(HOST_ARCH))
else
    $(call ndk_log,Host CPU from environment: $(HOST_ARCH))
endif

HOST_TAG := $(HOST_OS)-$(HOST_ARCH)

# If we are on Windows, we need to check that we are not running
# Cygwin 1.5, which is deprecated and won't run our toolchain
# binaries properly.
#
ifeq ($(HOST_TAG),windows-x86)
    # On cygwin, 'uname -r' returns something like 1.5.23(0.225/5/3)
    # We recognize 1.5. as the prefix to look for then.
    CYGWIN_VERSION := $(shell uname -r)
    ifneq ($(filter XX1.5.%,XX$(CYGWIN_VERSION)),)
        $(call __ndk_info,You seem to be running Cygwin 1.5, which is not supported.)
        $(call __ndk_info,Please upgrade to Cygwin 1.7 or higher.)
        $(call __ndk_error,Aborting.)
    endif
    # special-case the host-tag
    HOST_TAG := windows
endif
$(call ndk_log,HOST_TAG set to $(HOST_TAG))

#
# Verify that the 'awk' tool has the features we need.
# Both Nawk and Gawk do.
#
HOST_AWK := $(strip $(HOST_AWK))
ifndef HOST_AWK
    HOST_AWK := awk
    $(call ndk_log,Host awk tool was auto-detected: $(HOST_AWK))
else
    $(call ndk_log,Host awk tool from environment: $(HOST_AWK))
endif

# Location of all awk scripts we use
BUILD_AWK := $(NDK_ROOT)/build/awk

AWK_TEST := $(shell $(HOST_AWK) -f $(BUILD_AWK)/check-awk.awk)
$(call ndk_log,Host awk test returned: $(AWK_TEST))
ifneq ($(AWK_TEST),Pass)
    $(call __ndk_info,Host awk tool is outdated. Please define HOST_AWK to point to Gawk or Nawk !)
    $(call __ndk_error,Aborting.)
endif

# The location of the build system files
BUILD_SYSTEM := $(NDK_ROOT)/build/core

# Include common definitions
include $(BUILD_SYSTEM)/definitions.mk

# Location where all prebuilt binaries for a given host architectures
# will be stored.
HOST_PREBUILT := $(NDK_ROOT)/build/prebuilt/$(HOST_TAG)

# ====================================================================
#
# Read all toolchain-specific configuration files.
#
# Each toolchain must have a corresponding config.mk file located
# in build/toolchains/<name>/ that will be included here.
#
# Each one of these files should define the following variables:
#   TOOLCHAIN_NAME   toolchain name (e.g. arm-eabi-4.2.1)
#   TOOLCHAIN_ABIS   list of target ABIs supported by the toolchain.
#
# Then, it should include $(ADD_TOOLCHAIN) which will perform
# book-keeping for the build system.
#
# ====================================================================

# the build script to include in each toolchain config.mk
ADD_TOOLCHAIN := $(BUILD_SYSTEM)/add-toolchain.mk

# the list of all toolchains in this NDK
NDK_ALL_TOOLCHAINS :=
NDK_ALL_ABIS       :=

TOOLCHAIN_CONFIGS := $(wildcard $(NDK_ROOT)/build/toolchains/*/config.mk)
$(foreach _config_mk,$(TOOLCHAIN_CONFIGS),\
  $(eval include $(BUILD_SYSTEM)/add-toolchain.mk)\
)

NDK_ALL_TOOLCHAINS   := $(call uniq,$(NDK_ALL_TOOLCHAINS))
NDK_ALL_ABIS         := $(call uniq,$(NDK_ALL_ABIS))

# The default toolchain is now arm-eabi-4.4.0, however its
# C++ compiler is a tad bit more pedantic with certain
# constructs (e.g. templates) so allow users to switch back
# to the old 4.2.1 instead if they really want to.
#
# NOTE: you won't get armeabi-v7a support though !
#
NDK_TOOLCHAIN := $(strip $(NDK_TOOLCHAIN))
ifdef NDK_TOOLCHAIN
    # check that the toolchain name is supported
    $(if $(filter-out $(NDK_ALL_TOOLCHAINS),$(NDK_TOOLCHAIN)),\
      $(call __ndk_info,NDK_TOOLCHAIN is defined to the unsupported value $(NDK_TOOLCHAIN)) \
      $(call __ndk_info,Please use one of the following values: $(NDK_ALL_TOOLCHAINS))\
      $(call __ndk_error,Aborting)\
    ,)
    $(call ndk_log, Using specific toolchain $(NDK_TOOLCHAIN))
endif

$(call ndk_log, This NDK supports the following toolchains and target ABIs:)
$(foreach tc,$(NDK_ALL_TOOLCHAINS),\
    $(call ndk_log, $(space)$(space)$(tc):  $(NDK_TOOLCHAIN.$(tc).abis))\
)

# ====================================================================
#
# Read all platform-specific configuration files.
#
# Each platform must be located in build/platforms/android-<apilevel>
# where <apilevel> corresponds to an API level number, with:
#   3 -> Android 1.5
#   4 -> next platform release
#
# ====================================================================

NDK_PLATFORMS_ROOT := $(NDK_ROOT)/build/platforms
NDK_ALL_PLATFORMS := $(strip $(notdir $(wildcard $(NDK_PLATFORMS_ROOT)/android-*)))
$(call ndk_log,Found supported platforms: $(NDK_ALL_PLATFORMS))

$(foreach _platform,$(NDK_ALL_PLATFORMS),\
  $(eval include $(BUILD_SYSTEM)/add-platform.mk)\
)

# we're going to find the maximum platform number of the form android-<number>
# ignore others, which could correspond to special and experimental cases
NDK_ALL_PLATFORM_LEVELS := $(filter android-%,$(NDK_ALL_PLATFORMS))
NDK_ALL_PLATFORM_LEVELS := $(patsubst android-%,%,$(NDK_ALL_PLATFORM_LEVELS))
$(call ndk_log,Found stable platform levels: $(NDK_ALL_PLATFORM_LEVELS))

NDK_MAX_PLATFORM_LEVEL := 3
$(foreach level,$(NDK_ALL_PLATFORM_LEVELS),\
  $(eval NDK_MAX_PLATFORM_LEVEL := $$(call max,$$(NDK_MAX_PLATFORM_LEVEL),$$(level)))\
)
$(call ndk_log,Found max platform level: $(NDK_MAX_PLATFORM_LEVEL))

