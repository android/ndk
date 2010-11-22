# Copyright (C) 2008 The Android Open Source Project
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

# Check that LOCAL_MODULE is defined, then restore its LOCAL_XXXX values
$(call assert-defined,LOCAL_MODULE)
$(call module-restore-locals,$(LOCAL_MODULE))

# For now, only support target (device-specific modules).
# We may want to introduce support for host modules in the future
# but that is too experimental for now.
#
my := TARGET_

# LOCAL_MAKEFILE must also exist and name the Android.mk that
# included the module build script.
#
$(call assert-defined,LOCAL_MAKEFILE LOCAL_BUILD_SCRIPT LOCAL_BUILT_MODULE)

include $(BUILD_SYSTEM)/import-locals.mk

#
# Ensure that 'make <module>' and 'make clean-<module>' work
#
.PHONY: $(LOCAL_MODULE)
$(LOCAL_MODULE): $(LOCAL_BUILT_MODULE)

cleantarget := clean-$(LOCAL_MODULE)-$(TARGET_ARCH_ABI)
.PHONY: $(cleantarget)
clean: $(cleantarget)

$(cleantarget): PRIVATE_MODULE      := $(LOCAL_MODULE)
$(cleantarget): PRIVATE_TEXT        := [$(TARGET_ARCH_ABI)]
$(cleantarget): PRIVATE_CLEAN_FILES := $(LOCAL_BUILT_MODULE) \
                                       $($(my)OBJS)

$(cleantarget)::
	@echo "Clean: $(PRIVATE_MODULE) $(PRIVATE_TEXT)"
	$(hide) rm -rf $(PRIVATE_CLEAN_FILES)

ifeq ($(NDK_APP_DEBUGGABLE),true)
$(NDK_APP_GDBSETUP): PRIVATE_SRC_DIRS += $(LOCAL_C_INCLUDES) $(LOCAL_PATH)
endif

# list of generated object files
LOCAL_OBJECTS :=

# always define ANDROID when building binaries
#
LOCAL_CFLAGS := -DANDROID $(LOCAL_CFLAGS)

#
# Add the default system shared libraries to the build
#
ifeq ($(LOCAL_SYSTEM_SHARED_LIBRARIES),none)
  LOCAL_SHARED_LIBRARIES += $(TARGET_DEFAULT_SYSTEM_SHARED_LIBRARIES)
else
  LOCAL_SHARED_LIBRARIES += $(LOCAL_SYSTEM_SHARED_LIBRARIES)
endif


#
# Check LOCAL_CPP_EXTENSION, use '.cpp' by default
#
LOCAL_CPP_EXTENSION := $(strip $(LOCAL_CPP_EXTENSION))
ifeq ($(LOCAL_CPP_EXTENSION),)
  LOCAL_CPP_EXTENSION := .cpp
else
  ifneq ($(words $(LOCAL_CPP_EXTENSION)),1)
    $(call __ndk_info, LOCAL_CPP_EXTENSION in $(LOCAL_MAKEFILE) must be one word only, not '$(LOCAL_CPP_EXTENSION)')
    $(call __ndk_error, Aborting)
  endif
endif

#
# If LOCAL_ALLOW_UNDEFINED_SYMBOLS is not true, the linker will allow the generation
# of a binary that uses undefined symbols.
#
ifneq ($(LOCAL_ALLOW_UNDEFINED_SYMBOLS),true)
  LOCAL_LDFLAGS += $($(my)NO_UNDEFINED_LDFLAGS)
endif

# If LOCAL_DISABLE_NO_EXECUTE is not true, we disable generated code from running from
# the heap and stack by default.
#
ifndef ($(LOCAL_DISABLE_NO_EXECUTE),true)
  LOCAL_CFLAGS += $($(my)NO_EXECUTE_CFLAGS)
  LOCAL_LDFLAGS += $($(my)NO_EXECUTE_LDFLAGS)
endif

#
# The original Android build system allows you to use the .arm prefix
# to a source file name to indicate that it should be defined in either
# 'thumb' or 'arm' mode, depending on the value of LOCAL_ARM_MODE
#
# First, check LOCAL_ARM_MODE, it should be empty, 'thumb' or 'arm'
# We make the default 'thumb'
#
LOCAL_ARM_MODE := $(strip $(LOCAL_ARM_MODE))
ifdef LOCAL_ARM_MODE
  ifneq ($(words $(LOCAL_ARM_MODE)),1)
      $(call __ndk_info,   LOCAL_ARM_MODE in $(LOCAL_MAKEFILE) must be one word, not '$(LOCAL_ARM_MODE)')
      $(call __ndk_error, Aborting)
  endif
  # check that LOCAL_ARM_MODE is defined to either 'arm' or 'thumb'
  $(if $(filter-out thumb arm, $(LOCAL_ARM_MODE)),\
      $(call __ndk_info,   LOCAL_ARM_MODE must be defined to either 'arm' or 'thumb' in $(LOCAL_MAKEFILE) not '$(LOCAL_ARM_MODE)')\
      $(call __ndk_error, Aborting)\
  )
endif

# As a special case, the original Android build system
# allows one to specify that certain source files can be
# forced to build in ARM mode by using a '.arm' suffix
# after the extension, e.g.
#
#  LOCAL_SRC_FILES := foo.c.arm
#
# to build source file $(LOCAL_PATH)/foo.c as ARM
#

# As a special extension, the NDK also supports the .neon extension suffix
# to indicate that a single file can be compiled with ARM NEON support
# We must support both foo.c.neon and foo.c.arm.neon here
#
# Also, if LOCAL_ARM_NEON is set to 'true', force Neon mode for all source
# files
#

neon_sources  := $(filter %.neon,$(LOCAL_SRC_FILES))
neon_sources  := $(neon_sources:%.neon=%)

LOCAL_ARM_NEON := $(strip $(LOCAL_ARM_NEON))
ifdef LOCAL_ARM_NEON
  $(if $(filter-out true false,$(LOCAL_ARM_NEON)),\
    $(call __ndk_info,LOCAL_ARM_NEON must be defined either to 'true' or 'false' in $(LOCAL_MAKEFILE), not '$(LOCAL_ARM_NEON)')\
    $(call __ndk_error,Aborting) \
  )
endif
ifeq ($(LOCAL_ARM_NEON),true)
  neon_sources += $(LOCAL_SRC_FILES:%.neon=%))
endif

neon_sources := $(strip $(neon_sources))
ifdef neon_sources
  ifneq ($(TARGET_ARCH_ABI),armeabi-v7a)
    $(call __ndk_info,NEON support is only possible for armeabi-v7a ABI)
    $(call __ndk_info,Please add checks against TARGET_ARCH_ABI in $(LOCAL_MAKEFILE))
    $(call __ndk_error,Aborting)
  endif
  $(call tag-src-files,$(neon_sources:%.arm=%),neon)
endif

LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:%.neon=%)

# strip the .arm suffix from LOCAL_SRC_FILES
# and tag the relevant sources with the 'arm' tag
#
arm_sources     := $(filter %.arm,$(LOCAL_SRC_FILES))
arm_sources     := $(arm_sources:%.arm=%)
thumb_sources   := $(filter-out %.arm,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES := $(arm_sources) $(thumb_sources)

ifeq ($(LOCAL_ARM_MODE),arm)
    arm_sources := $(LOCAL_SRC_FILES)
endif
ifeq ($(LOCAL_ARM_MODE),thumb)
    arm_sources := $(empty)
endif
$(call tag-src-files,$(arm_sources),arm)

# Process all source file tags to determine toolchain-specific
# target compiler flags, and text.
#
$(call TARGET-process-src-files-tags)

# only call dump-src-file-tags during debugging
#$(dump-src-file-tags)

LOCAL_DEPENDENCY_DIRS :=
LOCAL_OBJECTS :=

# Build the sources to object files
#

$(foreach src,$(filter %.c,$(LOCAL_SRC_FILES)), $(call compile-c-source,$(src)))
$(foreach src,$(filter %.S %.s,$(LOCAL_SRC_FILES)), $(call compile-s-source,$(src)))

$(foreach src,$(filter %$(LOCAL_CPP_EXTENSION),$(LOCAL_SRC_FILES)),\
    $(call compile-cpp-source,$(src)))

unknown_sources := $(strip $(filter-out %.c %.S %.s %$(LOCAL_CPP_EXTENSION),$(LOCAL_SRC_FILES)))
ifdef unknown_sources
    $(call __ndk_info,WARNING: Unsupported source file extensions in $(LOCAL_MAKEFILE) for module $(LOCAL_MODULE))
    $(call __ndk_info,  $(unknown_sources))
endif

#
# The compile-xxx-source calls updated LOCAL_OBJECTS and LOCAL_DEPENDENCY_DIRS
#
ALL_DEPENDENCY_DIRS += $(sort $(LOCAL_DEPENDENCY_DIRS))
CLEAN_OBJS_DIRS     += $(LOCAL_OBJS_DIR)

#
# Handle the static and shared libraries this module depends on
#
LOCAL_STATIC_LIBRARIES       := $(call strip-lib-prefix,$(LOCAL_STATIC_LIBRARIES))
LOCAL_WHOLE_STATIC_LIBRARIES := $(call strip-lib-prefix,$(LOCAL_WHOLE_STATIC_LIBRARIES))
LOCAL_SHARED_LIBRARIES       := $(call strip-lib-prefix,$(LOCAL_SHARED_LIBRARIES))

static_libraries       := $(call map,module-get-built,$(LOCAL_STATIC_LIBRARIES))
whole_static_libraries := $(call map,module-get-built,$(LOCAL_WHOLE_STATIC_LIBRARIES))

shared_libraries := $(call map,module-get-built,$(LOCAL_SHARED_LIBRARIES))\
                    $(TARGET_PREBUILT_SHARED_LIBRARIES)

$(LOCAL_BUILT_MODULE): $(static_libraries) $(whole_static_libraries) $(shared_libraries)

# If LOCAL_LDLIBS contains anything like -l<library> then
# prepend a -L$(SYSROOT)/usr/lib to it to ensure that the linker
# looks in the right location
#
ifneq ($(filter -l%,$(LOCAL_LDLIBS)),)
    LOCAL_LDLIBS := -L$(call host-path,$(SYSROOT)/usr/lib) $(LOCAL_LDLIBS)
endif

$(LOCAL_BUILT_MODULE): PRIVATE_STATIC_LIBRARIES := $(static_libraries)
$(LOCAL_BUILT_MODULE): PRIVATE_WHOLE_STATIC_LIBRARIES := $(whole_static_libraries)
$(LOCAL_BUILT_MODULE): PRIVATE_SHARED_LIBRARIES := $(shared_libraries)
$(LOCAL_BUILT_MODULE): PRIVATE_OBJECTS          := $(LOCAL_OBJECTS)

$(LOCAL_BUILT_MODULE): PRIVATE_LDFLAGS := $(TARGET_LDFLAGS) $(LOCAL_LDFLAGS)
$(LOCAL_BUILT_MODULE): PRIVATE_LDLIBS  := $(LOCAL_LDLIBS) $(TARGET_LDLIBS)

$(LOCAL_BUILT_MODULE): PRIVATE_NAME := $(notdir $(LOCAL_BUILT_MODULE))

#
# If this is a static library module
#
ifeq ($(call module-get-class,$(LOCAL_MODULE)),STATIC_LIBRARY)
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	@ mkdir -p $(dir $@)
	@ echo "StaticLibrary  : $(PRIVATE_NAME)"
	$(hide) rm -rf $@
	$(hide) $(cmd-build-static-library)

ALL_STATIC_LIBRARIES += $(LOCAL_BUILT_MODULE)
endif

#
# If this is a shared library module
#
ifeq ($(call module-get-class,$(LOCAL_MODULE)),SHARED_LIBRARY)
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	@ mkdir -p $(dir $@)
	@ echo "SharedLibrary  : $(PRIVATE_NAME)"
	$(hide) $(cmd-build-shared-library)

ALL_SHARED_LIBRARIES += $(LOCAL_BUILT_MODULE)
endif

#
# If this is an executable module
#
ifeq ($(call module-get-class,$(LOCAL_MODULE)),EXECUTABLE)
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	@ mkdir -p $(dir $@)
	@ echo "Executable     : $(PRIVATE_NAME)"
	$(hide) $(cmd-build-executable)

ALL_EXECUTABLES += $(LOCAL_BUILT_MODULE)
endif

#
# If this is a prebuilt module
#
ifeq ($(call module-is-prebuilt,$(LOCAL_MODULE)),$(true))
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	@ mkdir -p $(dir $@)
	@ echo "Prebuilt       : $(PRIVATE_NAME) <= $(call pretty-dir,$(dir $<))"
	$(hide) cp -f $< $@
endif

#
# If this is an installable module
#
ifeq ($(call module-is-installable,$(LOCAL_MODULE)),$(true))
$(LOCAL_INSTALLED): PRIVATE_NAME    := $(notdir $(LOCAL_BUILT_MODULE))
$(LOCAL_INSTALLED): PRIVATE_SRC     := $(LOCAL_BUILT_MODULE)
$(LOCAL_INSTALLED): PRIVATE_DST_DIR := $(NDK_APP_DST_DIR)
$(LOCAL_INSTALLED): PRIVATE_DST     := $(LOCAL_INSTALLED)

$(LOCAL_INSTALLED): $(LOCAL_BUILT_MODULE) clean-installed-binaries
	@ echo "Install        : $(PRIVATE_NAME) => $(call pretty-dir,$(PRIVATE_DST))"
	$(hide) mkdir -p $(PRIVATE_DST_DIR)
	$(hide) install -p $(PRIVATE_SRC) $(PRIVATE_DST)
	$(hide) $(call cmd-strip, $(PRIVATE_DST))
endif
