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
# Common definitions for the Android NDK build system
#

# We use the GNU Make Standard Library
include $(NDK_ROOT)/build/gmsl/gmsl

# If NDK_TRACE is enabled then calls to the library functions are
# traced to stdout using warning messages with their arguments

ifdef NDK_TRACE
__ndk_tr1 = $(warning $0('$1'))
__ndk_tr2 = $(warning $0('$1','$2'))
__ndk_tr3 = $(warning $0('$1','$2','$3'))
else
__ndk_tr1 :=
__ndk_tr2 :=
__ndk_tr3 :=
endif

# -----------------------------------------------------------------------------
# Macro    : empty
# Returns  : an empty macro
# Usage    : $(empty)
# -----------------------------------------------------------------------------
empty :=

# -----------------------------------------------------------------------------
# Macro    : space
# Returns  : a single space
# Usage    : $(space)
# -----------------------------------------------------------------------------
space  := $(empty) $(empty)

# -----------------------------------------------------------------------------
# Function : last2
# Arguments: a list
# Returns  : the penultimate (next-to-last) element of a list
# Usage    : $(call last2, <LIST>)
# -----------------------------------------------------------------------------
last2 = $(word $(words $1), x $1)

# -----------------------------------------------------------------------------
# Function : last3
# Arguments: a list
# Returns  : the antepenultimate (second-next-to-last) element of a list
# Usage    : $(call last3, <LIST>)
# -----------------------------------------------------------------------------
last3 = $(word $(words $1), x x $1)

# -----------------------------------------------------------------------------
# Macro    : this-makefile
# Returns  : the name of the current Makefile in the inclusion stack
# Usage    : $(this-makefile)
# -----------------------------------------------------------------------------
this-makefile = $(lastword $(MAKEFILE_LIST))

# -----------------------------------------------------------------------------
# Macro    : local-makefile
# Returns  : the name of the last parsed Android.mk file
# Usage    : $(local-makefile)
# -----------------------------------------------------------------------------
local-makefile = $(lastword $(filter %Android.mk,$(MAKEFILE_LIST)))

# -----------------------------------------------------------------------------
# Function : assert-defined
# Arguments: 1: list of variable names
# Returns  : None
# Usage    : $(call assert-defined, VAR1 VAR2 VAR3...)
# Rationale: Checks that all variables listed in $1 are defined, or abort the
#            build
# -----------------------------------------------------------------------------
assert-defined = $(foreach __varname,$(strip $1),\
  $(if $(strip $($(__varname))),,\
    $(call __ndk_error, Assertion failure: $(__varname) is not defined)\
  )\
)

# -----------------------------------------------------------------------------
# Function : clear-vars
# Arguments: 1: list of variable names
#            2: file where the variable should be defined
# Returns  : None
# Usage    : $(call clear-vars, VAR1 VAR2 VAR3...)
# Rationale: Clears/undefines all variables in argument list
# -----------------------------------------------------------------------------
clear-vars = $(foreach __varname,$1,$(eval $(__varname) := $(empty)))

# -----------------------------------------------------------------------------
# Function : check-required-vars
# Arguments: 1: list of variable names
#            2: file where the variable(s) should be defined
# Returns  : None
# Usage    : $(call check-required-vars, VAR1 VAR2 VAR3..., <file>)
# Rationale: Checks that all required vars listed in $1 were defined by $2
#            or abort the build with an error
# -----------------------------------------------------------------------------
check-required-vars = $(foreach __varname,$1,\
  $(if $(strip $($(__varname))),,\
    $(call __ndk_info, Required variable $(__varname) is not defined by $2)\
    $(call __ndk_error,Aborting)\
  )\
)

# =============================================================================
#
# Modules database
#
# The following declarations are used to manage the list of modules
# defined in application's Android.mk files.
#
# Technical note:
#    We use __ndk_modules to hold the list of all modules corresponding
#    to a given application.
#
#    For each module 'foo', __ndk_modules.foo.<field> is used
#    to store module-specific information.
#
#        android_mk   -> points to the Android.mk where the module is defined
#        type         -> type of module (e.g. 'static', 'shared', ...)
#        built        -> location of module built file (e.g. out/apps/<app>/<abi>/libfoo.so)
#        installed    -> location of module installation (e.g. $PROJECT/libs/<abi>/libfoo.so)
#        depends      -> list of other modules this module depends on
#
#    Note that some modules are never installed (e.g. static libraries).
#
# =============================================================================

# the list of managed fields per module
modules-fields = type android_mk built installed depends

# -----------------------------------------------------------------------------
# Function : modules-clear
# Arguments: None
# Returns  : None
# Usage    : $(call modules-clear)
# Rationale: clears the list of defined modules known by the build system
# -----------------------------------------------------------------------------
modules-clear = \
    $(foreach __mod,$(__ndk_modules),\
        $(foreach __field,$(modules-fields),\
            $(eval __ndk_modules.$(__mod).$(__field) := $(empty))\
        )\
    )\
    $(eval __ndk_modules := $(empty_set))

# -----------------------------------------------------------------------------
# Function : modules-get-list
# Arguments: None
# Returns  : The list of all recorded modules
# Usage    : $(call modules-get-list)
# -----------------------------------------------------------------------------
modules-get-list = $(__ndk_modules)

# -----------------------------------------------------------------------------
# Function : modules-add
# Arguments: 1: module name
#            2: built module path
#            3: path to Android.mk where the module is defined
#            4: type of module (e.g. 'static')
# Returns  : None
# Usage    : $(call modules-add,<modulename>,<Android.mk path>)
# Rationale: add a new module. If it is already defined, print an error message
#            and abort.
# -----------------------------------------------------------------------------
module-add = \
  $(if $(call set_is_member,$(__ndk_modules),$1),\
       $(call __ndk_info,Trying to define local module '$1' in $3.)\
       $(call __ndk_info,But this module was already defined by $(__ndk_modules.$1.android_mk).)\
       $(call __ndk_error,Aborting.)\
  )\
  $(eval __ndk_modules := $(call set_insert,$(__ndk_modules),$1))\
  $(eval __ndk_modules.$1.android_mk := $3)\
  $(eval __ndk_modules.$1.type       := $4)\
  $(eval __ndk_modules.$1.built      := $2)\

module-add-static-library = $(call module-add,$1,$2,$3,static)
module-add-shared-library = $(call module-add,$1,$2,$3,shared)
module-add-executable     = $(call module-add,$1,$2,$3,executable)

# Returns $(true) iff module $1 is of type $2
module-is-type = $(call seq,$(__ndk_modules.$1.type),$2)

# Retrieve built location of module $1
module-get-built = $(__ndk_modules.$1.built)

# Returns $(true) is module $1 is of a given type
module-is-static-library = $(call module-is-type,$1,static)
module-is-shared-library = $(call module-is-type,$1,shared)
module-is-exectuable     = $(call module-is-type,$1,executable)
module-is-installable    = $(call or,$(call module-is-type,$1,shared),$(call module-is-type,$1,executable))

# Dump all module information. Only use this for debugging
modules-dump-database = \
    $(info Modules: $(__ndk_modules)) \
    $(foreach __mod,$(__ndk_modules),\
        $(info $(space)$(space)$(__mod):)\
        $(foreach __field,$(modules-fields),\
            $(info $(space)$(space)$(space)$(space)$(__field): $(__ndk_modules.$(__mod).$(__field)))\
        )\
    )\
    $(info --- end of list)


# -----------------------------------------------------------------------------
# Function : module-add-static-depends
# Arguments: 1: module name
#            2: list/set of static library modules this module depends on.
# Returns  : None
# Usage    : $(call module-add-static-depends,<modulename>,<list of module names>)
# Rationale: Record that a module depends on a set of static libraries.
#            Use module-get-static-dependencies to retrieve final list.
# -----------------------------------------------------------------------------
module-add-static-depends = \
    $(call modules-add-depends-any,$1,$2,depends) \

# -----------------------------------------------------------------------------
# Function : module-add-shared-depends
# Arguments: 1: module name
#            2: list/set of shared library modules this module depends on.
# Returns  : None
# Usage    : $(call module-add-shared-depends,<modulename>,<list of module names>)
# Rationale: Record that a module depends on a set of shared libraries.
#            Use modulge-get-shared-dependencies to retrieve final list.
# -----------------------------------------------------------------------------
module-add-shared-depends = \
    $(call modules-add-depends-any,$1,$2,depends) \

# Used internally by module-add-static-depends and module-add-shared-depends
# NOTE: this function must not modify the existing dependency order when new depends are added.
#
modules-add-depends-any = \
    $(eval __ndk_modules.$1.$3 += $(filter-out $(__ndk_modules.$1.$3),$(call strip-lib-prefix,$2)))

# -----------------------------------------------------------------------------
# Function : module-set-installed
# Arguments: 1: module name
#            2: installation path for the module
# Returns  : None
# Usage    : $(call module-set-installed,<module>,<installed>)
# Rationale: Records the installed path of a given module. Can later be retrieved
#            with module-get-installed.
# -----------------------------------------------------------------------------
module-set-installed = \
    $(eval __ndk_modules.$1.installed := $2)

# -----------------------------------------------------------------------------
# Function : module-get-installed
# Arguments: 1: module name
# Returns  : Path of installed locaiton
# Usage    : $(call module-get-installed,<module>)
# -----------------------------------------------------------------------------
module-get-installed = $(__ndk_modules.$1.installed)

# -----------------------------------------------------------------------------
# Function : modules-get-all-dependencies
# Arguments: 1: list of module names
# Returns  : List of all the modules $1 depends on transitively.
# Usage    : $(call modules-all-get-dependencies,<list of module names>)
# Rationale: This computes the closure of all module dependencies starting from $1
# -----------------------------------------------------------------------------
module-get-all-dependencies = \
    $(strip $(call modules-get-closure,$1,depends))

modules-get-closure = \
    $(eval __closure_deps  := $(strip $1)) \
    $(eval __closure_wq    := $(__closure_deps)) \
    $(eval __closure_field := $(strip $2)) \
    $(call modules-closure)\
    $(__closure_deps)

# Used internally by modules-get-dependencies
# Note the tricky use of conditional recursion to work around the fact that
# the GNU Make language does not have any conditional looping construct
# like 'while'.
#
modules-closure = \
    $(eval __closure_mod := $(call first,$(__closure_wq))) \
    $(eval __closure_wq  := $(call rest,$(__closure_wq))) \
    $(eval __closure_new := $(filter-out $(__closure_deps),$(__ndk_modules.$(__closure_mod).$(__closure_field))))\
    $(eval __closure_deps += $(__closure_new)) \
    $(eval __closure_wq   := $(strip $(__closure_wq) $(__closure_new)))\
    $(if $(__closure_wq),$(call modules-closure)) \


# =============================================================================
#
# Utility functions
#
# =============================================================================

# -----------------------------------------------------------------------------
# Function : check-user-define
# Arguments: 1: name of variable that must be defined by the user
#            2: name of Makefile where the variable should be defined
#            3: name/description of the Makefile where the check is done, which
#               must be included by $2
# Returns  : None
# -----------------------------------------------------------------------------
check-user-define = $(if $(strip $($1)),,\
  $(call __ndk_error,Missing $1 before including $3 in $2))

# -----------------------------------------------------------------------------
# This is used to check that LOCAL_MODULE is properly defined by an Android.mk
# file before including one of the $(BUILD_SHARED_LIBRARY), etc... files.
#
# Function : check-user-LOCAL_MODULE
# Arguments: 1: name/description of the included build Makefile where the
#               check is done
# Returns  : None
# Usage    : $(call check-user-LOCAL_MODULE, BUILD_SHARED_LIBRARY)
# -----------------------------------------------------------------------------
check-defined-LOCAL_MODULE = \
  $(call check-user-define,LOCAL_MODULE,$(local-makefile),$(1)) \
  $(if $(call seq,$(words $(LOCAL_MODULE)),1),,\
    $(call __ndk_info,LOCAL_MODULE definition in $(local-makefile) must not contain space)\
    $(call __ndk_error,Please correct error. Aborting)\
  )

# -----------------------------------------------------------------------------
# Strip any 'lib' prefix in front of a given string.
#
# Function : strip-lib-prefix
# Arguments: 1: module name
# Returns  : module name, without any 'lib' prefix if any
# Usage    : $(call strip-lib-prefix,$(LOCAL_MODULE))
# -----------------------------------------------------------------------------
strip-lib-prefix = $(1:lib%=%)

# -----------------------------------------------------------------------------
# This is used to strip any lib prefix from LOCAL_MODULE, then check that
# the corresponding module name is not already defined.
#
# Function : check-user-LOCAL_MODULE
# Arguments: 1: path of Android.mk where this LOCAL_MODULE is defined
# Returns  : None
# Usage    : $(call check-LOCAL_MODULE,$(LOCAL_MAKEFILE))
# -----------------------------------------------------------------------------
check-LOCAL_MODULE = \
  $(eval LOCAL_MODULE := $$(call strip-lib-prefix,$$(LOCAL_MODULE)))

# -----------------------------------------------------------------------------
# Macro    : my-dir
# Returns  : the directory of the current Makefile
# Usage    : $(my-dir)
# -----------------------------------------------------------------------------
my-dir = $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

# -----------------------------------------------------------------------------
# Function : all-makefiles-under
# Arguments: 1: directory path
# Returns  : a list of all makefiles immediately below some directory
# Usage    : $(call all-makefiles-under, <some path>)
# -----------------------------------------------------------------------------
all-makefiles-under = $(wildcard $1/*/Android.mk)

# -----------------------------------------------------------------------------
# Macro    : all-subdir-makefiles
# Returns  : list of all makefiles in subdirectories of the current Makefile's
#            location
# Usage    : $(all-subdir-makefiles)
# -----------------------------------------------------------------------------
all-subdir-makefiles = $(call all-makefiles-under,$(call my-dir))

# =============================================================================
#
# Source file tagging support.
#
# Each source file listed in LOCAL_SRC_FILES can have any number of
# 'tags' associated to it. A tag name must not contain space, and its
# usage can vary.
#
# For example, the 'debug' tag is used to sources that must be built
# in debug mode, the 'arm' tag is used for sources that must be built
# using the 32-bit instruction set on ARM platforms, and 'neon' is used
# for sources that must be built with ARM Advanced SIMD (a.k.a. NEON)
# support.
#
# More tags might be introduced in the future.
#
#  LOCAL_SRC_TAGS contains the list of all tags used (initially empty)
#  LOCAL_SRC_FILES contains the list of all source files.
#  LOCAL_SRC_TAG.<tagname> contains the set of source file names tagged
#      with <tagname>
#  LOCAL_SRC_FILES_TAGS.<filename> contains the set of tags for a given
#      source file name
#
# Tags are processed by a toolchain-specific function (e.g. TARGET-compute-cflags)
# which will call various functions to compute source-file specific settings.
# These are currently stored as:
#
#  LOCAL_SRC_FILES_TARGET_CFLAGS.<filename> contains the list of
#      target-specific C compiler flags used to compile a given
#      source file. This is set by the function TARGET-set-cflags
#      defined in the toolchain's setup.mk script.
#
#  LOCAL_SRC_FILES_TEXT.<filename> contains the 'text' that will be
#      displayed along the label of the build output line. For example
#      'thumb' or 'arm  ' with ARM-based toolchains.
#
# =============================================================================

# -----------------------------------------------------------------------------
# Macro    : clear-all-src-tags
# Returns  : remove all source file tags and associated data.
# Usage    : $(clear-all-src-tags)
# -----------------------------------------------------------------------------
clear-all-src-tags = \
$(foreach __tag,$(LOCAL_SRC_TAGS), \
    $(eval LOCAL_SRC_TAG.$(__tag) := $(empty)) \
) \
$(foreach __src,$(LOCAL_SRC_FILES), \
    $(eval LOCAL_SRC_FILES_TAGS.$(__src) := $(empty)) \
    $(eval LOCAL_SRC_FILES_TARGET_CFLAGS.$(__src) := $(empty)) \
    $(eval LOCAL_SRC_FILES_TEXT.$(__src) := $(empty)) \
) \
$(eval LOCAL_SRC_TAGS := $(empty_set))

# -----------------------------------------------------------------------------
# Macro    : tag-src-files
# Arguments: 1: list of source files to tag
#            2: tag name (must not contain space)
# Usage    : $(call tag-src-files,<list-of-source-files>,<tagname>)
# Rationale: Add a tag to a list of source files
# -----------------------------------------------------------------------------
tag-src-files = \
$(eval LOCAL_SRC_TAGS := $(call set_insert,$2,$(LOCAL_SRC_TAGS))) \
$(eval LOCAL_SRC_TAG.$2 := $(call set_union,$1,$(LOCAL_SRC_TAG.$2))) \
$(foreach __src,$1, \
    $(eval LOCAL_SRC_FILES_TAGS.$(__src) += $2) \
)

# -----------------------------------------------------------------------------
# Macro    : get-src-files-with-tag
# Arguments: 1: tag name
# Usage    : $(call get-src-files-with-tag,<tagname>)
# Return   : The list of source file names that have been tagged with <tagname>
# -----------------------------------------------------------------------------
get-src-files-with-tag = $(LOCAL_SRC_TAG.$1)

# -----------------------------------------------------------------------------
# Macro    : get-src-files-without-tag
# Arguments: 1: tag name
# Usage    : $(call get-src-files-without-tag,<tagname>)
# Return   : The list of source file names that have NOT been tagged with <tagname>
# -----------------------------------------------------------------------------
get-src-files-without-tag = $(filter-out $(LOCAL_SRC_TAG.$1),$(LOCAL_SRC_FILES))

# -----------------------------------------------------------------------------
# Macro    : set-src-files-target-cflags
# Arguments: 1: list of source files
#            2: list of compiler flags
# Usage    : $(call set-src-files-target-cflags,<sources>,<flags>)
# Rationale: Set or replace the set of compiler flags that will be applied
#            when building a given set of source files. This function should
#            normally be called from the toolchain-specific function that
#            computes all compiler flags for all source files.
# -----------------------------------------------------------------------------
set-src-files-target-cflags = $(foreach __src,$1,$(eval LOCAL_SRC_FILES_TARGET_CFLAGS.$(__src) := $2))

# -----------------------------------------------------------------------------
# Macro    : add-src-files-target-cflags
# Arguments: 1: list of source files
#            2: list of compiler flags
# Usage    : $(call add-src-files-target-cflags,<sources>,<flags>)
# Rationale: A variant of set-src-files-target-cflags that can be used
#            to append, instead of replace, compiler flags for specific
#            source files.
# -----------------------------------------------------------------------------
add-src-files-target-cflags = $(foreach __src,$1,$(eval LOCAL_SRC_FILES_TARGET_CFLAGS.$(__src) += $2))

# -----------------------------------------------------------------------------
# Macro    : get-src-file-target-cflags
# Arguments: 1: single source file name
# Usage    : $(call get-src-file-target-cflags,<source>)
# Rationale: Return the set of target-specific compiler flags that must be
#            applied to a given source file. These must be set prior to this
#            call using set-src-files-target-cflags or add-src-files-target-cflags
# -----------------------------------------------------------------------------
get-src-file-target-cflags = $(LOCAL_SRC_FILES_TARGET_CFLAGS.$1)

# -----------------------------------------------------------------------------
# Macro    : set-src-files-text
# Arguments: 1: list of source files
#            2: text
# Usage    : $(call set-src-files-text,<sources>,<text>)
# Rationale: Set or replace the 'text' associated to a set of source files.
#            The text is a very short string that complements the build
#            label. For example, it will be either 'thumb' or 'arm  ' for
#            ARM-based toolchains. This function must be called by the
#            toolchain-specific functions that processes all source files.
# -----------------------------------------------------------------------------
set-src-files-text = $(foreach __src,$1,$(eval LOCAL_SRC_FILES_TEXT.$(__src) := $2))

# -----------------------------------------------------------------------------
# Macro    : get-src-file-text
# Arguments: 1: single source file
# Usage    : $(call get-src-file-text,<source>)
# Rationale: Return the 'text' associated to a given source file when
#            set-src-files-text was called.
# -----------------------------------------------------------------------------
get-src-file-text = $(LOCAL_SRC_FILES_TEXT.$1)

# This should only be called for debugging the source files tagging system
dump-src-file-tags = \
$(info LOCAL_SRC_TAGS := $(LOCAL_SRC_TAGS)) \
$(info LOCAL_SRC_FILES = $(LOCAL_SRC_FILES)) \
$(foreach __tag,$(LOCAL_SRC_TAGS),$(info LOCAL_SRC_TAG.$(__tag) = $(LOCAL_SRC_TAG.$(__tag)))) \
$(foreach __src,$(LOCAL_SRC_FILES),$(info LOCAL_SRC_FILES_TAGS.$(__src) = $(LOCAL_SRC_FILES_TAGS.$(__src)))) \
$(info WITH arm = $(call get-src-files-with-tag,arm)) \
$(info WITHOUT arm = $(call get-src-files-without-tag,arm)) \
$(foreach __src,$(LOCAL_SRC_FILES),$(info LOCAL_SRC_FILES_TARGET_CFLAGS.$(__src) = $(LOCAL_SRC_FILES_TARGET_CFLAGS.$(__src)))) \
$(foreach __src,$(LOCAL_SRC_FILES),$(info LOCAL_SRC_FILES_TEXT.$(__src) = $(LOCAL_SRC_FILES_TEXT.$(__src)))) \


# =============================================================================
#
# Application.mk support
#
# =============================================================================

# the list of variables that *must* be defined in Application.mk files
NDK_APP_VARS_REQUIRED :=

# the list of variables that *may* be defined in Application.mk files
NDK_APP_VARS_OPTIONAL := APP_OPTIM APP_CPPFLAGS APP_CFLAGS APP_CXXFLAGS \
                         APP_PLATFORM APP_BUILD_SCRIPT APP_ABI APP_MODULES \
                         APP_PROJECT_PATH

# the list of all variables that may appear in an Application.mk file
# or defined by the build scripts.
NDK_APP_VARS := $(NDK_APP_VARS_REQUIRED) \
                $(NDK_APP_VARS_OPTIONAL) \
                APP_DEBUGGABLE \
                APP_MANIFEST

# =============================================================================
#
# Android.mk support
#
# =============================================================================


# =============================================================================
#
# Generated files support
#
# =============================================================================


# -----------------------------------------------------------------------------
# Function  : host-static-library-path
# Arguments : 1: library module name (e.g. 'foo')
# Returns   : location of generated host library name (e.g. '..../libfoo.a)
# Usage     : $(call host-static-library-path,<modulename>)
# -----------------------------------------------------------------------------
host-static-library-path = $(HOST_OUT)/lib$1.a

# -----------------------------------------------------------------------------
# Function  : host-executable-path
# Arguments : 1: executable module name (e.g. 'foo')
# Returns   : location of generated host executable name (e.g. '..../foo)
# Usage     : $(call host-executable-path,<modulename>)
# -----------------------------------------------------------------------------
host-executable-path = $(HOST_OUT)/$1$(HOST_EXE)

# -----------------------------------------------------------------------------
# Function  : static-library-path
# Arguments : 1: library module name (e.g. 'foo')
# Returns   : location of generated static library name (e.g. '..../libfoo.a)
# Usage     : $(call static-library-path,<modulename>)
# -----------------------------------------------------------------------------
static-library-path = $(TARGET_OUT)/lib$1.a

# -----------------------------------------------------------------------------
# Function  : shared-library-path
# Arguments : 1: library module name (e.g. 'foo')
# Returns   : location of generated shared library name (e.g. '..../libfoo.so)
# Usage     : $(call shared-library-path,<modulename>)
# -----------------------------------------------------------------------------
shared-library-path = $(TARGET_OUT)/lib$1.so

# -----------------------------------------------------------------------------
# Function  : executable-path
# Arguments : 1: executable module name (e.g. 'foo')
# Returns   : location of generated exectuable name (e.g. '..../foo)
# Usage     : $(call executable-path,<modulename>)
# -----------------------------------------------------------------------------
executable-path = $(TARGET_OUT)/$1

# =============================================================================
#
# Build commands support
#
# =============================================================================

# -----------------------------------------------------------------------------
# Macro    : hide
# Returns  : nothing
# Usage    : $(hide)<make commands>
# Rationale: To be used as a prefix for Make build commands to hide them
#            by default during the build. To show them, set V=1 in your
#            environment or command-line.
#
#            For example:
#
#                foo.o: foo.c
#                -->|$(hide) <build-commands>
#
#            Where '-->|' stands for a single tab character.
#
# -----------------------------------------------------------------------------
ifeq ($(V),1)
hide = $(empty)
else
hide = @
endif

# -----------------------------------------------------------------------------
# Template  : ev-compile-c-source
# Arguments : 1: single C source file name (relative to LOCAL_PATH)
#             2: target object file (without path)
# Returns   : None
# Usage     : $(eval $(call ev-compile-c-source,<srcfile>,<objfile>)
# Rationale : Internal template evaluated by compile-c-source and
#             compile-s-source
# -----------------------------------------------------------------------------
define  ev-compile-c-source
_SRC:=$$(LOCAL_PATH)/$(1)
_OBJ:=$$(LOCAL_OBJS_DIR)/$(2)

$$(_OBJ): PRIVATE_SRC      := $$(_SRC)
$$(_OBJ): PRIVATE_OBJ      := $$(_OBJ)
$$(_OBJ): PRIVATE_MODULE   := $$(LOCAL_MODULE)
$$(_OBJ): PRIVATE_ARM_MODE := $$(LOCAL_ARM_MODE)
$$(_OBJ): PRIVATE_ARM_TEXT := $$(call get-src-file-text,$1)
$$(_OBJ): PRIVATE_CC       := $$($$(my)CC)
$$(_OBJ): PRIVATE_CFLAGS   := $$($$(my)CFLAGS) \
                              $$(call get-src-file-target-cflags,$(1)) \
                              $$(LOCAL_C_INCLUDES:%=-I%) \
                              -I$$(LOCAL_PATH) \
                              $$(LOCAL_CFLAGS) \
                              $$(NDK_APP_CFLAGS)

$$(_OBJ): $$(_SRC) $$(LOCAL_MAKEFILE) $$(NDK_APP_APPLICATION_MK)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile $$(PRIVATE_ARM_TEXT)  : $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC)"
	$(hide) $$(PRIVATE_CC) $$(PRIVATE_CFLAGS) -c \
	-MMD -MP -MF $$(PRIVATE_OBJ).d \
	$$(PRIVATE_SRC) \
	-o $$(PRIVATE_OBJ)

LOCAL_OBJECTS         += $$(_OBJ)
LOCAL_DEPENDENCY_DIRS += $$(dir $$(_OBJ))
endef

# -----------------------------------------------------------------------------
# Function  : compile-c-source
# Arguments : 1: single C source file name (relative to LOCAL_PATH)
# Returns   : None
# Usage     : $(call compile-c-source,<srcfile>)
# Rationale : Setup everything required to build a single C source file
# -----------------------------------------------------------------------------
compile-c-source = $(eval $(call ev-compile-c-source,$1,$(1:%.c=%.o)))

# -----------------------------------------------------------------------------
# Function  : compile-s-source
# Arguments : 1: single Assembly source file name (relative to LOCAL_PATH)
# Returns   : None
# Usage     : $(call compile-s-source,<srcfile>)
# Rationale : Setup everything required to build a single Assembly source file
# -----------------------------------------------------------------------------
compile-s-source = $(eval $(call ev-compile-c-source,$1,$(1:%.S=%.o)))


# -----------------------------------------------------------------------------
# Template  : ev-compile-cpp-source
# Arguments : 1: single C++ source file name (relative to LOCAL_PATH)
#             2: target object file (without path)
# Returns   : None
# Usage     : $(eval $(call ev-compile-cpp-source,<srcfile>,<objfile>)
# Rationale : Internal template evaluated by compile-cpp-source
# -----------------------------------------------------------------------------

define  ev-compile-cpp-source
_SRC:=$$(LOCAL_PATH)/$(1)
_OBJ:=$$(LOCAL_OBJS_DIR)/$(2)

$$(_OBJ): PRIVATE_SRC      := $$(_SRC)
$$(_OBJ): PRIVATE_OBJ      := $$(_OBJ)
$$(_OBJ): PRIVATE_MODULE   := $$(LOCAL_MODULE)
$$(_OBJ): PRIVATE_ARM_MODE := $$(LOCAL_ARM_MODE)
$$(_OBJ): PRIVATE_ARM_TEXT := $$(call get-src-file-text,$1)
$$(_OBJ): PRIVATE_CXX      := $$($$(my)CXX)
$$(_OBJ): PRIVATE_CXXFLAGS := $$($$(my)CXXFLAGS) \
                              $$(call get-src-file-target-cflags,$(1)) \
                              $$(LOCAL_C_INCLUDES:%=-I%) \
                              -I$$(LOCAL_PATH) \
                              $$(LOCAL_CFLAGS) \
                              $$(LOCAL_CPPFLAGS) \
                              $$(LOCAL_CXXFLAGS) \
                              $$(NDK_APP_CFLAGS) \
                              $$(NDK_APP_CPPFLAGS) \
                              $$(NDK_APP_CXXFLAGS) \

$$(_OBJ): $$(_SRC) $$(LOCAL_MAKEFILE) $$(NDK_APP_APPLICATION_MK)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "Compile++ $$(PRIVATE_ARM_TEXT): $$(PRIVATE_MODULE) <= $$(PRIVATE_SRC)"
	$(hide) $$(PRIVATE_CXX) $$(PRIVATE_CXXFLAGS) -c \
	-MMD -MP -MF $$(PRIVATE_OBJ).d \
	$$(PRIVATE_SRC) \
	-o $$(PRIVATE_OBJ)

LOCAL_OBJECTS         += $$(_OBJ)
LOCAL_DEPENDENCY_DIRS += $$(dir $$(_OBJ))
endef

# -----------------------------------------------------------------------------
# Function  : compile-cpp-source
# Arguments : 1: single C++ source file name (relative to LOCAL_PATH)
# Returns   : None
# Usage     : $(call compile-c-source,<srcfile>)
# Rationale : Setup everything required to build a single C++ source file
# -----------------------------------------------------------------------------
compile-cpp-source = $(eval $(call ev-compile-cpp-source,$1,$(1:%$(LOCAL_CPP_EXTENSION)=%.o)))

# -----------------------------------------------------------------------------
# Command   : cmd-install-file
# Arguments : 1: source file
#             2: destination file
# Returns   : None
# Usage     : $(call cmd-install-file,<srcfile>,<dstfile>)
# Rationale : To be used as a Make build command to copy/install a file to
#             a given location.
# -----------------------------------------------------------------------------
define cmd-install-file
@mkdir -p $(dir $2)
$(hide) cp -fp $1 $2
endef

