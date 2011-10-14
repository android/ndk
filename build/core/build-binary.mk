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
	@$(HOST_ECHO) "Clean: $(PRIVATE_MODULE) $(PRIVATE_TEXT)"
	$(hide) $(call host-rmdir,$(PRIVATE_CLEAN_FILES))

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
bad_cpp_extensions := $(strip $(filter-out .%,$(LOCAL_CPP_EXTENSION)))
ifdef bad_cpp_extensions
    $(call __ndk_info,WARNING: Invalid LOCAL_CPP_EXTENSION values: $(bad_cpp_extensions))
    LOCAL_CPP_EXTENSION := $(filter $(bad_cpp_extensions),$(LOCAL_CPP_EXTENSIONS))
endif
LOCAL_CPP_EXTENSION := $(strip $(LOCAL_CPP_EXTENSION))
ifeq ($(LOCAL_CPP_EXTENSION),)
  LOCAL_CPP_EXTENSION := .cpp
else
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
  neon_sources += $(LOCAL_SRC_FILES:%.neon=%)
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
LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:%.arm=%)

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

# all_source_patterns contains the list of filename patterns that correspond
# to source files recognized by our build system
all_source_extensions := .c .s .S $(LOCAL_CPP_EXTENSION)
all_source_patterns   := $(foreach _ext,$(all_source_extensions),%$(_ext))
all_cpp_patterns      := $(foreach _ext,$(LOCAL_CPP_EXTENSION),%$(_ext))

unknown_sources := $(strip $(filter-out $(all_source_patterns),$(LOCAL_SRC_FILES)))
ifdef unknown_sources
    $(call __ndk_info,WARNING: Unsupported source file extensions in $(LOCAL_MAKEFILE) for module $(LOCAL_MODULE))
    $(call __ndk_info,  $(unknown_sources))
endif

# LOCAL_OBJECTS will list all object files corresponding to the sources
# listed in LOCAL_SRC_FILES, in the *same* order.
#
LOCAL_OBJECTS := $(LOCAL_SRC_FILES)
$(foreach _ext,$(all_source_extensions),\
    $(eval LOCAL_OBJECTS := $$(LOCAL_OBJECTS:%$(_ext)=%.o))\
)
LOCAL_OBJECTS := $(filter %.o,$(LOCAL_OBJECTS))
LOCAL_OBJECTS := $(subst ../,__/,$(LOCAL_OBJECTS))
LOCAL_OBJECTS := $(foreach _obj,$(LOCAL_OBJECTS),$(LOCAL_OBJS_DIR)/$(_obj))

# If the module has any kind of C++ features, enable them in LOCAL_CPPFLAGS
#
ifneq (,$(call module-has-c++-features,$(LOCAL_MODULE),rtti))
    LOCAL_CPPFLAGS += -frtti
endif
ifneq (,$(call module-has-c++-features,$(LOCAL_MODULE),exceptions))
    LOCAL_CPPFLAGS += -fexceptions
endif

# If we're using the 'system' STL and use rtti or exceptions, then
# automatically link against the GNU libsupc++ for now.
#
ifneq (,$(call module-has-c++-features,$(LOCAL_MODULE),rtti exceptions))
    ifeq (system,$(NDK_APP_STL))
      LOCAL_LDLIBS := $(LOCAL_LDLIBS) $(NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/libsupc++.a
    endif
endif

# Build the sources to object files
#

$(foreach src,$(filter %.c,$(LOCAL_SRC_FILES)), $(call compile-c-source,$(src),$(call get-object-name,$(src))))
$(foreach src,$(filter %.S %.s,$(LOCAL_SRC_FILES)), $(call compile-s-source,$(src),$(call get-object-name,$(src))))

$(foreach src,$(filter $(all_cpp_patterns),$(LOCAL_SRC_FILES)),\
    $(call compile-cpp-source,$(src),$(call get-object-name,$(src)))\
)

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

# Transitive closure of static libraries
LOCAL_STATIC_LIBRARIES       := $(call module-get-depends,$(LOCAL_STATIC_LIBRARIES),STATIC_LIBRARIES)
LOCAL_WHOLE_STATIC_LIBRARIES := $(call module-get-depends,$(LOCAL_WHOLE_STATIC_LIBRARIES),WHOLE_STATIC_LIBRARIES)

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
$(LOCAL_BUILT_MODULE): PRIVATE_LIBGCC := $(TARGET_LIBGCC)

$(LOCAL_BUILT_MODULE): PRIVATE_LD := $(TARGET_LD)
$(LOCAL_BUILT_MODULE): PRIVATE_LDFLAGS := $(TARGET_LDFLAGS) $(LOCAL_LDFLAGS)
$(LOCAL_BUILT_MODULE): PRIVATE_LDLIBS  := $(LOCAL_LDLIBS) $(TARGET_LDLIBS)

$(LOCAL_BUILT_MODULE): PRIVATE_NAME := $(notdir $(LOCAL_BUILT_MODULE))
$(LOCAL_BUILT_MODULE): PRIVATE_CXX := $(TARGET_CXX)
$(LOCAL_BUILT_MODULE): PRIVATE_CC := $(TARGET_CC)
$(LOCAL_BUILT_MODULE): PRIVATE_AR := $(TARGET_AR) $(TARGET_ARFLAGS)
$(LOCAL_BUILT_MODULE): PRIVATE_SYSROOT := $(SYSROOT)

#
# If this is a static library module
#
ifeq ($(call module-get-class,$(LOCAL_MODULE)),STATIC_LIBRARY)
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	@ $(call host-mkdir,$(dir $@))
	@ $(HOST_ECHO) "StaticLibrary  : $(PRIVATE_NAME)"
	$(hide) $(call host-rm,$@)
	$(hide) $(cmd-build-static-library)

ALL_STATIC_LIBRARIES += $(LOCAL_BUILT_MODULE)
endif

#
# If this is a shared library module
#
ifeq ($(call module-get-class,$(LOCAL_MODULE)),SHARED_LIBRARY)
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	@ $(call host-mkdir,$(dir $@))
	@ $(HOST_ECHO) "SharedLibrary  : $(PRIVATE_NAME)"
	$(hide) $(cmd-build-shared-library)

ALL_SHARED_LIBRARIES += $(LOCAL_BUILT_MODULE)
endif

#
# If this is an executable module
#
ifeq ($(call module-get-class,$(LOCAL_MODULE)),EXECUTABLE)
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	@ $(call host-mkdir,$(dir $@))
	@ $(HOST_ECHO) "Executable     : $(PRIVATE_NAME)"
	$(hide) $(cmd-build-executable)

ALL_EXECUTABLES += $(LOCAL_BUILT_MODULE)
endif

#
# If this is a prebuilt module
#
ifeq ($(call module-is-prebuilt,$(LOCAL_MODULE)),$(true))
$(LOCAL_BUILT_MODULE): $(LOCAL_OBJECTS)
	@ $(call host-mkdir,$(dir $@))
	@ $(HOST_ECHO) "Prebuilt       : $(PRIVATE_NAME) <= $(call pretty-dir,$(dir $<))"
	$(hide) $(call host-cp,$<,$@)
endif

#
# If this is an installable module
#
ifeq ($(call module-is-installable,$(LOCAL_MODULE)),$(true))
$(LOCAL_INSTALLED): PRIVATE_NAME    := $(notdir $(LOCAL_BUILT_MODULE))
$(LOCAL_INSTALLED): PRIVATE_SRC     := $(LOCAL_BUILT_MODULE)
$(LOCAL_INSTALLED): PRIVATE_DST_DIR := $(NDK_APP_DST_DIR)
$(LOCAL_INSTALLED): PRIVATE_DST     := $(LOCAL_INSTALLED)
$(LOCAL_INSTALLED): PRIVATE_STRIP   := $(TARGET_STRIP)

$(LOCAL_INSTALLED): $(LOCAL_BUILT_MODULE) clean-installed-binaries
	@$(HOST_ECHO) "Install        : $(PRIVATE_NAME) => $(call pretty-dir,$(PRIVATE_DST))"
	$(hide) $(call host-mkdir,$(PRIVATE_DST_DIR))
	$(hide) $(call host-install,$(PRIVATE_SRC),$(PRIVATE_DST))
	$(hide) $(call cmd-strip, $(PRIVATE_DST))
endif
