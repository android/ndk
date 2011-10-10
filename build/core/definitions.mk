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

space4 := $(space)$(space)$(space)$(space)

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
# Function : remove-duplicates
# Arguments: a list
# Returns  : the list with duplicate items removed, order is preserved.
# Usage    : $(call remove-duplicates, <LIST>)
# Note     : This is equivalent to the 'uniq' function provided by GMSL,
#            however this implementation is non-recursive and *much*
#            faster. It will also not explode the stack with a lot of
#            items like 'uniq' does.
# -----------------------------------------------------------------------------
remove-duplicates = $(strip \
  $(eval __uniq_ret :=) \
  $(foreach __uniq_item,$1,\
    $(if $(findstring $(__uniq_item),$(__uniq_ret)),,\
      $(eval __uniq_ret += $(__uniq_item))\
    )\
  )\
  $(__uniq_ret))

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

# -----------------------------------------------------------------------------
# Function : host-path
# Arguments: 1: file path
# Returns  : file path, as understood by the host file system
# Usage    : $(call host-path,<path>)
# Rationale: This function is used to translate Cygwin paths into
#            Cygwin-specific ones. On other platforms, it will just
#            return its argument.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),cygwin)
host-path = $(if $(strip $1),$(call cygwin-to-host-path,$1))
else
host-path = $1
endif

# -----------------------------------------------------------------------------
# Function : host-c-includes
# Arguments: 1: list of file paths (e.g. "foo bar")
# Returns  : list of include compiler options (e.g. "-Ifoo -Ibar")
# Usage    : $(call host-c-includes,<paths>)
# Rationale: This function is used to translate Cygwin paths into
#            Cygwin-specific ones. On other platforms, it will just
#            return its argument.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),cygwin)
host-c-includes = $(patsubst %,-I%,$(call host-path,$1))
else
host-c-includes = $(1:%=-I%)
endif


# -----------------------------------------------------------------------------
# Function : link-whole-archives
# Arguments: 1: list of whole static libraries
# Returns  : linker flags to use the whole static libraries
# Usage    : $(call link-whole-archives,<libraries>)
# Rationale: This function is used to put the list of whole static libraries
#            inside a -Wl,--whole-archive ... -Wl,--no-whole-archive block.
#            If the list is empty, it returns an empty string.
#            This function also calls host-path to translate the library
#            paths.
# -----------------------------------------------------------------------------
link-whole-archives = $(if $(strip $1),$(call link-whole-archive-flags,$1))
link-whole-archive-flags = -Wl,--whole-archive $(call host-path,$1) -Wl,--no-whole-archive

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
#        type         -> type of module (e.g. 'static', 'shared', ...)
#        depends      -> list of other modules this module depends on
#
#    Also, LOCAL_XXXX values defined for a module are recorded in XXXX, e.g.:
#
#        PATH   -> recorded LOCAL_PATH for the module
#        CFLAGS -> recorded LOCAL_CFLAGS for the module
#        ...
#
#    Some of these are created by build scripts like BUILD_STATIC_LIBRARY:
#
#        MAKEFILE -> The Android.mk where the module is defined.
#        LDFLAGS  -> Final linker flags
#        OBJECTS  -> List of module objects
#        BUILT_MODULE -> location of module built file (e.g. obj/<app>/<abi>/libfoo.so)
#
#    Note that some modules are never installed (e.g. static libraries).
#
# =============================================================================

# The list of LOCAL_XXXX variables that are recorded for each module definition
# These are documented by docs/ANDROID-MK.TXT. Exception is LOCAL_MODULE
#
modules-LOCALS := \
    MODULE \
    MODULE_FILENAME \
    PATH \
    SRC_FILES \
    CPP_EXTENSION \
    C_INCLUDES \
    CFLAGS \
    CXXFLAGS \
    CPPFLAGS \
    STATIC_LIBRARIES \
    WHOLE_STATIC_LIBRARIES \
    SHARED_LIBRARIES \
    LDLIBS \
    ALLOW_UNDEFINED_SYMBOLS \
    ARM_MODE \
    ARM_NEON \
    DISABLE_NO_EXECUTE \
    EXPORT_CFLAGS \
    EXPORT_CPPFLAGS \
    EXPORT_LDLIBS \
    EXPORT_C_INCLUDES \
    FILTER_ASM \
    CPP_FEATURES \

# The following are generated by the build scripts themselves

# LOCAL_MAKEFILE will contain the path to the Android.mk defining the module
modules-LOCALS += MAKEFILE

# LOCAL_LDFLAGS will contain the set of final linker flags for the module
modules-LOCALS += LDFLAGS

# LOCAL_OBJECTS will contain the list of object files generated from the
# module's sources, if any.
modules-LOCALS += OBJECTS

# LOCAL_BUILT_MODULE will contain the location of the symbolic version of
# the generated module (i.e. the one containing all symbols used during
# native debugging). It is generally under $PROJECT/obj/local/
modules-LOCALS += BUILT_MODULE

# LOCAL_OBJS_DIR will contain the location where the object files for
# this module will be stored. Usually $PROJECT/obj/local/<module>/obj
modules-LOCALS += OBJS_DIR

# LOCAL_INSTALLED will contain the location of the installed version
# of the module. Usually $PROJECT/libs/<abi>/<prefix><module><suffix>
# where <prefix> and <suffix> depend on the module class.
modules-LOCALS += INSTALLED

# LOCAL_MODULE_CLASS will contain the type of the module
# (e.g. STATIC_LIBRARY, SHARED_LIBRARY, etc...)
modules-LOCALS += MODULE_CLASS

# the list of managed fields per module
modules-fields = depends \
                 $(modules-LOCALS)

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
    $(eval __ndk_modules := $(empty_set)) \
    $(eval __ndk_top_modules := $(empty)) \
    $(eval __ndk_import_list := $(empty)) \
    $(eval __ndk_import_depth := $(empty))

# -----------------------------------------------------------------------------
# Function : modules-get-list
# Arguments: None
# Returns  : The list of all recorded modules
# Usage    : $(call modules-get-list)
# -----------------------------------------------------------------------------
modules-get-list = $(__ndk_modules)

# -----------------------------------------------------------------------------
# Function : modules-get-top-list
# Arguments: None
# Returns  : The list of all recorded non-imported modules
# Usage    : $(call modules-get-top-list)
# -----------------------------------------------------------------------------
modules-get-top-list = $(__ndk_top_modules)

# -----------------------------------------------------------------------------
# Function : module-add
# Arguments: 1: module name
# Returns  : None
# Usage    : $(call module-add,<modulename>)
# Rationale: add a new module. If it is already defined, print an error message
#            and abort. This will record all LOCAL_XXX variables for the module.
# -----------------------------------------------------------------------------
module-add = \
  $(call assert-defined,LOCAL_MAKEFILE LOCAL_BUILT_MODULE LOCAL_OBJS_DIR LOCAL_MODULE_CLASS)\
  $(if $(call set_is_member,$(__ndk_modules),$1),\
    $(call __ndk_info,Trying to define local module '$1' in $(LOCAL_MAKEFILE).)\
    $(call __ndk_info,But this module was already defined by $(__ndk_modules.$1.MAKEFILE).)\
    $(call __ndk_error,Aborting.)\
  )\
  $(eval __ndk_modules := $(call set_insert,$(__ndk_modules),$1))\
  $(if $(strip $(__ndk_import_depth)),,\
    $(eval __ndk_top_modules := $(call set_insert,$(__ndk_top_modules),$1))\
  )\
  $(if $(call module-class-is-installable,$(LOCAL_MODULE_CLASS)),\
    $(eval LOCAL_INSTALLED := $(NDK_APP_DST_DIR)/$(notdir $(LOCAL_BUILT_MODULE)))\
  )\
  $(foreach __local,$(modules-LOCALS),\
    $(eval __ndk_modules.$1.$(__local) := $(LOCAL_$(__local)))\
  )\
  $(call module-handle-c++-features,$1)


# Retrieve the class of module $1
module-get-class = $(__ndk_modules.$1.MODULE_CLASS)

# Retrieve built location of module $1
module-get-built = $(__ndk_modules.$1.BUILT_MODULE)

# Returns $(true) is module $1 is installable
# An installable module is one that will be copied to $PROJECT/libs/<abi>/
# (e.g. shared libraries).
#
module-is-installable = $(call module-class-is-installable,$(call module-get-class,$1))

# Returns $(true) if module $1 is prebuilt
# A prebuilt module is one declared with BUILD_PREBUILT_SHARED_LIBRARY or
# BUILD_PREBUILT_STATIC_LIBRARY
#
module-is-prebuilt = $(call module-class-is-prebuilt,$(call module-get-class,$1))

# -----------------------------------------------------------------------------
# Function : module-get-export
# Arguments: 1: module name
#            2: export variable name without LOCAL_EXPORT_ prefix (e.g. 'CFLAGS')
# Returns  : Exported value
# Usage    : $(call module-get-export,<modulename>,<varname>)
# Rationale: Return the recorded value of LOCAL_EXPORT_$2, if any, for module $1
# -----------------------------------------------------------------------------
module-get-export = $(__ndk_modules.$1.EXPORT_$2)

# -----------------------------------------------------------------------------
# Function : module-get-listed-export
# Arguments: 1: list of module names
#            2: export variable name without LOCAL_EXPORT_ prefix (e.g. 'CFLAGS')
# Returns  : Exported values
# Usage    : $(call module-get-listed-export,<module-list>,<varname>)
# Rationale: Return the recorded value of LOCAL_EXPORT_$2, if any, for modules
#            listed in $1.
# -----------------------------------------------------------------------------
module-get-listed-export = $(strip \
    $(foreach __listed_module,$1,\
        $(call module-get-export,$(__listed_module),$2)\
    ))

# -----------------------------------------------------------------------------
# Function : modules-restore-locals
# Arguments: 1: module name
# Returns  : None
# Usage    : $(call module-restore-locals,<modulename>)
# Rationale: Restore the recorded LOCAL_XXX definitions for a given module.
# -----------------------------------------------------------------------------
module-restore-locals = \
    $(foreach __local,$(modules-LOCALS),\
        $(eval LOCAL_$(__local) := $(__ndk_modules.$1.$(__local)))\
    )

# Dump all module information. Only use this for debugging
modules-dump-database = \
    $(info Modules: $(__ndk_modules)) \
    $(foreach __mod,$(__ndk_modules),\
        $(info $(space4)$(__mod):)\
        $(foreach __field,$(modules-fields),\
            $(eval __fieldval := $(strip $(__ndk_modules.$(__mod).$(__field))))\
            $(if $(__fieldval),\
                $(if $(filter 1,$(words $(__fieldval))),\
                    $(info $(space4)$(space4)$(__field): $(__fieldval)),\
                    $(info $(space4)$(space4)$(__field): )\
                    $(foreach __fielditem,$(__fieldval),\
                        $(info $(space4)$(space4)$(space4)$(__fielditem))\
                    )\
                )\
            )\
        )\
    )\
    $(info --- end of modules list)


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
    $(call module-add-depends-any,$1,$2,depends) \

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
    $(call module-add-depends-any,$1,$2,depends) \

# Used internally by module-add-static-depends and module-add-shared-depends
# NOTE: this function must not modify the existing dependency order when new depends are added.
#
module-add-depends-any = \
    $(eval __ndk_modules.$1.$3 += $(filter-out $(__ndk_modules.$1.$3),$(call strip-lib-prefix,$2)))

# Used to recompute all dependencies once all module information has been recorded.
#
modules-compute-dependencies = \
    $(foreach __module,$(__ndk_modules),\
        $(call module-compute-depends,$(__module))\
    )

module-compute-depends = \
    $(call module-add-static-depends,$1,$(__ndk_modules.$1.STATIC_LIBRARIES))\
    $(call module-add-static-depends,$1,$(__ndk_modules.$1.WHOLE_STATIC_LIBRARIES))\
    $(call module-add-shared-depends,$1,$(__ndk_modules.$1.SHARED_LIBRARIES))\

module-get-installed = $(__ndk_modules.$1.INSTALLED)

# -----------------------------------------------------------------------------
# Function : modules-get-all-dependencies
# Arguments: 1: list of module names
# Returns  : List of all the modules $1 depends on transitively.
# Usage    : $(call modules-all-get-dependencies,<list of module names>)
# Rationale: This computes the closure of all module dependencies starting from $1
# -----------------------------------------------------------------------------
module-get-all-dependencies = $(strip \
    $(call modules-get-closure,$1,depends))

modules-get-closure = \
    $(eval __closure_deps  := $(strip $(call strip-lib-prefix,$1))) \
    $(eval __closure_wq    := $(__closure_deps)) \
    $(eval __closure_field := $(strip $2)) \
    $(call modules-closure)\
    $(__closure_deps)

# Used internally by modules-get-all-dependencies
# Note the tricky use of conditional recursion to work around the fact that
# the GNU Make language does not have any conditional looping construct
# like 'while'.
#
modules-closure = \
    $(eval __closure_mod := $(call first,$(__closure_wq))) \
    $(eval __closure_wq  := $(call rest,$(__closure_wq))) \
    $(eval __closure_val := $(call strip-lib-prefix,$(__ndk_modules.$(__closure_mod).$(__closure_field)))) \
    $(eval __closure_new := $(filter-out $(__closure_deps),$(__closure_val)))\
    $(eval __closure_deps += $(__closure_new)) \
    $(eval __closure_wq   := $(strip $(__closure_wq) $(__closure_new)))\
    $(if $(__closure_wq),$(call modules-closure)) \

# -----------------------------------------------------------------------------
# Function : module-get-depends
# Arguments: 1: list of module names
#            2: local module type (e.g. SHARED_LIBRARIES)
# Returns  : List all the <local-type> modules $1 depends on transitively.
# Usage    : $(call module-get-depends,<list of module names>,<local-type>)
# Rationale: This computes the closure of all local module dependencies starting from $1
# -----------------------------------------------------------------------------
module-get-depends = $(strip $(call modules-get-closure,$1,$2))


# -----------------------------------------------------------------------------
# Function : modules-get-all-installable
# Arguments: 1: list of module names
# Returns  : List of all the installable modules $1 depends on transitively.
# Usage    : $(call modules-all-get-installable,<list of module names>)
# Rationale: This computes the closure of all installable module dependencies starting from $1
# -----------------------------------------------------------------------------
# For now, only the closure of LOCAL_SHARED_LIBRARIES is enough
modules-get-all-installable = $(strip \
    $(foreach __alldep,$(call module-get-depends,$1,depends),\
        $(if $(call module-is-installable,$(__alldep)),$(__alldep))\
    ))

# Return the C++ extension of a given module
# $1: module name
module-get-cpp-extension = $(strip \
    $(if $(__ndk_modules.$1.CPP_EXTENSION),\
        $(__ndk_modules.$1.CPP_EXTENSION),\
        .cpp\
    ))

# Return the list of C++ sources of a given module
#
module-get-c++-sources = \
    $(filter %$(call module-get-cpp-extension,$1),$(__ndk_modules.$1.SRC_FILES))

# Returns true if a module has C++ sources
#
module-has-c++-sources = $(strip $(call module-get-c++-sources,$1))


# Add C++ dependencies to any module that has C++ sources.
# $1: list of C++ runtime static libraries (if any)
# $2: list of C++ runtime shared libraries (if any)
#
modules-add-c++-dependencies = \
    $(foreach __module,$(__ndk_modules),\
        $(if $(call module-has-c++-sources,$(__module)),\
            $(call ndk_log,Module '$(__module)' has C++ sources)\
            $(call module-add-c++-deps,$(__module),$1,$2),\
        )\
    )


# Return the compiler flags used to compile a C++ module
# Order matters and should match the one used by the build command
module-get-c++-flags = $(strip \
    $(__ndk_modules.$1.CFLAGS) \
    $(__ndk_modules.$1.CPPFLAGS) \
    $(__ndk_modules.$1.CXXFLAGS))

# This function is used to remove certain flags from a module compiler flags
# $1: Module name
# $2: List of flags to remove
#
module-filter-out-compiler-flags = \
    $(eval __ndk_modules.$1.CFLAGS   := $(filter-out $2,$(__ndk_modules.$1.CFLAGS)))\
    $(eval __ndk_modules.$1.CPPFLAGS := $(filter-out $2,$(__ndk_modules.$1.CPPFLAGS)))\
    $(eval __ndk_modules.$1.CXXFLAGS := $(filter-out $2,$(__ndk_modules.$1.CXXFLAGS)))

# Return true if a module's compiler flags enable rtti
# We just look at -frtti and -fno-rtti on the command-line
# and keep the last one of these flags.
module-flags-have-rtti = $(strip \
        $(filter -frtti,\
            $(lastword $(filter -frtti -fno-rtti,$(call module-get-c++-flags,$1)))\
        )\
    )

# Same with C++ exception support (i.e. -fexceptions and -fno-exceptions)
#
module-flags-have-exceptions = $(strip \
        $(filter -fexceptions,\
            $(lastword $(filter -fexceptions -fno-execeptions,$(call module-get-c++-flags,$1)))\
        )\
    )

# Handle the definition of LOCAL_CPP_FEATURES, i.e.:
#
#  - If it is defined, check that it only contains valid values
#  - If it is undefined, try to compute its value automatically by
#    looking at the C++ compiler flags used to build the module
#
# After this, we remove all features flags from the module's command-line
# And add only the correct ones back in LOCAL_CPP_FLAGS
#
module-handle-c++-features = \
    $(if $(strip $(__ndk_modules.$1.CPP_FEATURES)),\
        $(eval __cxxbad := $(filter-out rtti exceptions,$(__ndk_modules.$1.CPP_FEATURES)))\
        $(if $(__cxxbad),\
            $(call __ndk_info,WARNING: Ignoring invalid values in LOCAL_CPP_FEATURES definition in $(__ndk_modules.$1.MAKEFILE): $(__cxxbad))\
            $(eval __ndk_modules.$1.CPP_FEATURES := $(strip $(filter-out $(__cxxbad),$(__ndk_modules.$1.CPP_FEATURES))))\
        )\
    ,\
        $(eval __ndk_modules.$1.CPP_FEATURES := $(strip \
            $(if $(call module-flags-have-rtti,$1),rtti) \
            $(if $(call module-flags-have-exceptions,$1),exceptions) \
        )) \
    )\
    $(call module-filter-out-compiler-flags,$1,-frtti -fno-rtti -fexceptions -fno-exceptions)\

# Returns true if a module or its dependencies have specific C++ features
# (i.e. RTTI or Exceptions)
#
# $1: module name
# $2: list of features (e.g. 'rtti' or 'exceptions')
#
module-has-c++-features = $(strip \
    $(eval __cxxdeps  := $(call module-get-all-dependencies,$1))\
    $(eval __cxxflags := $(foreach __cxxdep,$(__cxxdeps),$(__ndk_modules.$(__cxxdep).CPP_FEATURES)))\
    $(if $(filter $2,$(__cxxflags)),true,)\
    )

# Add standard C++ dependencies to a given module
#
# $1: module name
# $2: list of C++ runtime static libraries (if any)
# $3: list of C++ runtime shared libraries (if any)
#
module-add-c++-deps = \
    $(eval __ndk_modules.$1.STATIC_LIBRARIES += $(2))\
    $(eval __ndk_modules.$1.SHARED_LIBRARIES += $(3))


# =============================================================================
#
# Utility functions
#
# =============================================================================

# -----------------------------------------------------------------------------
# Function : parent-dir
# Arguments: 1: path
# Returns  : Parent dir or path of $1, with final separator removed.
# -----------------------------------------------------------------------------
parent-dir = $(patsubst %/,%,$(dir $1))


# -----------------------------------------------------------------------------
# Function : pretty-dir
# Arguments: 1: path
# Returns  : Remove NDK_PROJECT_PATH prefix from a given path. This can be
#            used to perform pretty-printing for logs.
# -----------------------------------------------------------------------------
pretty-dir = $(patsubst $(NDK_ROOT)/%,<NDK>/%,\
                 $(patsubst $(NDK_PROJECT_PATH)/%,%,$1))

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
# This is used to check that LOCAL_MODULE_FILENAME, if defined, is correct.
#
# Function : check-user-LOCAL_MODULE_FILENAME
# Returns  : None
# Usage    : $(call check-user-LOCAL_MODULE_FILENAME)
# -----------------------------------------------------------------------------
check-LOCAL_MODULE_FILENAME = \
  $(if $(strip $(LOCAL_MODULE_FILENAME)),\
    $(if $(call seq,$(words $(LOCAL_MODULE_FILENAME)),1),,\
        $(call __ndk_info,$(LOCAL_MAKEFILE):$(LOCAL_MODULE): LOCAL_MODULE_FILENAME must not contain spaces)\
        $(call __ndk_error,Plase correct error. Aborting)\
    )\
    $(if $(filter %.a %.so,$(LOCAL_MODULE_FILENAME)),\
        $(call __ndk_info,$(LOCAL_MAKEFILE):$(LOCAL_MODULE): LOCAL_MODULE_FILENAME should not include file extensions)\
    )\
  )

# -----------------------------------------------------------------------------
# Function  : handle-module-filename
# Arguments : 1: default file prefix
#             2: file suffix
# Returns   : None
# Usage     : $(call handle-module-filename,<prefix>,<suffix>)
# Rationale : To be used to check and or set the module's filename through
#             the LOCAL_MODULE_FILENAME variable.
# -----------------------------------------------------------------------------
handle-module-filename = $(eval $(call ev-handle-module-filename,$1,$2))

#
# Check that LOCAL_MODULE_FILENAME is properly defined
# - with one single item
# - without a library file extension
# - with no directory separators
#
define ev-check-module-filename
ifneq (1,$$(words $$(LOCAL_MODULE_FILENAME)))
    $$(call __ndk_info,$$(LOCAL_MAKEFILE):$$(LOCAL_MODULE): LOCAL_MODULE_FILENAME must not contain any space)
    $$(call __ndk_error,Aborting)
endif
ifneq (,$$(filter %.a %.so,$$(LOCAL_MODULE_FILENAME)))
    $$(call __ndk_info,$$(LOCAL_MAKEFILE):$$(LOCAL_MODULE): LOCAL_MODULE_FILENAME must not contain a file extension)
    $$(call __ndk_error,Aborting)
endif
ifneq (1,$$(words $$(subst /, ,$$(LOCAL_MODULE_FILENAME))))
    $$(call __ndk_info,$$(LOCAL_MAKEFILE):$$(LOCAL_MODULE): LOCAL_MODULE_FILENAME must not contain directory separators)
    $$(call __ndk_error,Aborting)
endif
endef

#
# Check the definition of LOCAL_MODULE_FILENAME. If none exists,
# infer it from the LOCAL_MODULE name.
#
# $1: default file prefix
# $2: default file suffix
#
define ev-handle-module-filename
LOCAL_MODULE_FILENAME := $$(strip $$(LOCAL_MODULE_FILENAME))
ifndef LOCAL_MODULE_FILENAME
    LOCAL_MODULE_FILENAME := $1$$(LOCAL_MODULE)
endif
$$(eval $$(call ev-check-module-filename))
LOCAL_MODULE_FILENAME := $$(LOCAL_MODULE_FILENAME)$2
endef

handle-prebuilt-module-filename = $(eval $(call ev-handle-prebuilt-module-filename,$1))

#
# Check the definition of LOCAL_MODULE_FILENAME for a _prebuilt_ module.
# If none exists, infer it from $(LOCAL_SRC_FILES)
#
# $1: default file suffix
#
define ev-handle-prebuilt-module-filename
LOCAL_MODULE_FILENAME := $$(strip $$(LOCAL_MODULE_FILENAME))
ifndef LOCAL_MODULE_FILENAME
    LOCAL_MODULE_FILENAME := $$(notdir $(LOCAL_SRC_FILES))
    LOCAL_MODULE_FILENAME := $$(LOCAL_MODULE_FILENAME:%.a=%)
    LOCAL_MODULE_FILENAME := $$(LOCAL_MODULE_FILENAME:%.so=%)
endif
LOCAL_MODULE_FILENAME := $$(LOCAL_MODULE_FILENAME)$1
$$(eval $$(call ev-check-module-filename))
endef


# -----------------------------------------------------------------------------
# Function  : handle-module-built
# Returns   : None
# Usage     : $(call handle-module-built)
# Rationale : To be used to automatically compute the location of the generated
#             binary file, and the directory where to place its object files.
# -----------------------------------------------------------------------------
handle-module-built = \
    $(eval LOCAL_BUILT_MODULE := $(TARGET_OUT)/$(LOCAL_MODULE_FILENAME))\
    $(eval LOCAL_OBJS_DIR     := $(TARGET_OBJS)/$(LOCAL_MODULE))

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
my-dir = $(call parent-dir,$(lastword $(MAKEFILE_LIST)))

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
                         APP_PROJECT_PATH APP_STL

# the list of all variables that may appear in an Application.mk file
# or defined by the build scripts.
NDK_APP_VARS := $(NDK_APP_VARS_REQUIRED) \
                $(NDK_APP_VARS_OPTIONAL) \
                APP_DEBUG \
                APP_DEBUGGABLE \
                APP_MANIFEST

# =============================================================================
#
# Android.mk support
#
# =============================================================================

# =============================================================================
#
# Build commands support
#
# =============================================================================

get-object-name = $(strip \
    $(subst ../,__/,\
        $(eval __obj := $1)\
        $(foreach __ext,.c .s .S $(LOCAL_CPP_EXTENSION),\
            $(eval __obj := $(__obj:%$(__ext)=%.o))\
        )\
        $(__obj)\
    ))

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

# cmd-convert-deps
#
# On Cygwin, we need to convert the .d dependency file generated by
# the gcc toolchain by transforming any WIndows paths inside it into
# Cygwin paths that GNU Make can understand (i.e. C:/Foo => /cygdrive/c/Foo)
#
# To do that, we will force the compiler to write the dependency file to
# <foo>.d.org, which will will later convert through a clever Awk script.
#
# The result will be written to <foo>.d and <foo>.d.org erased.
# Note that it is important to use different file names here.
#
#
# On other systems, we simply tell the compiler to write to the .d file directly.
#
# NOTE: In certain cases, no dependency file will be generated by the
#       compiler (e.g. when compiling an assembly file as foo.s)
#
# convert-deps is used to compute the name of the compiler-generated dependency file
# cmd-convert-deps is a command used to convert it to a Cygwin-specific path
#
ifeq ($(HOST_OS),cygwin)
convert-deps = $1.org
cmd-convert-deps = \
    && ( if [ -f "$1.org" ]; then \
          $(HOST_AWK) -f $(BUILD_AWK)/convert-deps-to-cygwin.awk $1.org > $1 && \
          rm -f $1.org; \
      fi )
else
convert-deps = $1
cmd-convert-deps = 
endif

# This assumes that many variables have been pre-defined:
# _SRC: source file
# _OBJ: destination file
# _CC: 'compiler' command
# _FLAGS: 'compiler' flags
# _TEXT: Display text (e.g. "Compile++ thumb", must be EXACTLY 15 chars long)
#
define ev-build-file
$$(_OBJ): PRIVATE_SRC      := $$(_SRC)
$$(_OBJ): PRIVATE_OBJ      := $$(_OBJ)
$$(_OBJ): PRIVATE_DEPS     := $$(call host-path,$$(_OBJ).d)
$$(_OBJ): PRIVATE_MODULE   := $$(LOCAL_MODULE)
$$(_OBJ): PRIVATE_TEXT     := "$$(_TEXT)"
$$(_OBJ): PRIVATE_CC       := $$(_CC)
$$(_OBJ): PRIVATE_CFLAGS   := $$(_FLAGS)
$$(_OBJ): $$(_SRC) $$(LOCAL_MAKEFILE) $$(NDK_APP_APPLICATION_MK)
	@mkdir -p $$(dir $$(PRIVATE_OBJ))
	@echo "$$(PRIVATE_TEXT)  : $$(PRIVATE_MODULE) <= $$(notdir $$(PRIVATE_SRC))"
	$(hide) $$(PRIVATE_CC) -MMD -MP -MF $$(call convert-deps,$$(PRIVATE_DEPS)) $$(PRIVATE_CFLAGS) $$(call host-path,$$(PRIVATE_SRC)) -o $$(call host-path,$$(PRIVATE_OBJ)) \
	$$(call cmd-convert-deps,$$(PRIVATE_DEPS))
endef

# This assumes the same things than ev-build-file, but will handle
# the definition of LOCAL_FILTER_ASM as well.
define ev-build-source-file
LOCAL_DEPENDENCY_DIRS += $$(dir $$(_OBJ))
ifndef LOCAL_FILTER_ASM
  # Trivial case: Directly generate an object file
  $$(eval $$(call ev-build-file))
else
  # This is where things get hairy, we first transform
  # the source into an assembler file, send it to the
  # filter, then generate a final object file from it.
  #

  # First, remember the original settings and compute
  # the location of our temporary files.
  #
  _ORG_SRC := $$(_SRC)
  _ORG_OBJ := $$(_OBJ)
  _ORG_FLAGS := $$(_FLAGS)
  _ORG_TEXT  := $$(_TEXT)

  _OBJ_ASM_ORIGINAL := $$(patsubst %.o,%.s,$$(_ORG_OBJ))
  _OBJ_ASM_FILTERED := $$(patsubst %.o,%.filtered.s,$$(_ORG_OBJ))

  # If the source file is a plain assembler file, we're going to
  # use it directly in our filter.
  ifneq (,$$(filter %.s,$$(_SRC)))
    _OBJ_ASM_ORIGINAL := $$(_SRC)
  endif

  #$$(info SRC=$$(_SRC) OBJ=$$(_OBJ) OBJ_ORIGINAL=$$(_OBJ_ASM_ORIGINAL) OBJ_FILTERED=$$(_OBJ_ASM_FILTERED))

  # We need to transform the source into an assembly file, instead of
  # an object. The proper way to do that depends on the file extension.
  #
  # For C and C++ source files, simply replace the -c by an -S in the
  # compilation command (this forces the compiler to generate an
  # assembly file).
  #
  # For assembler templates (which end in .S), replace the -c with -E
  # to send it to the preprocessor instead.
  #
  # Don't do anything for plain assembly files (which end in .s)
  #
  ifeq (,$$(filter %.s,$$(_SRC)))
    _OBJ   := $$(_OBJ_ASM_ORIGINAL)
    ifneq (,$$(filter %.S,$$(_SRC)))
      _FLAGS := $$(patsubst -c,-E,$$(_ORG_FLAGS))
    else
      _FLAGS := $$(patsubst -c,-S,$$(_ORG_FLAGS))
    endif
    $$(eval $$(call ev-build-file))
  endif

  # Next, process the assembly file with the filter
  $$(_OBJ_ASM_FILTERED): PRIVATE_SRC    := $$(_OBJ_ASM_ORIGINAL)
  $$(_OBJ_ASM_FILTERED): PRIVATE_DST    := $$(_OBJ_ASM_FILTERED)
  $$(_OBJ_ASM_FILTERED): PRIVATE_FILTER := $$(LOCAL_FILTER_ASM)
  $$(_OBJ_ASM_FILTERED): PRIVATE_MODULE := $$(LOCAL_MODULE)
  $$(_OBJ_ASM_FILTERED): $$(_OBJ_ASM_ORIGINAL)
	@echo "AsmFilter      : $$(PRIVATE_MODULE) <= $$(notdir $$(PRIVATE_SRC))"
	$(hide) $$(PRIVATE_FILTER) $$(PRIVATE_SRC) $$(PRIVATE_DST)

  # Then, generate the final object, we need to keep assembler-specific
  # flags which look like -Wa,<option>:
  _SRC   := $$(_OBJ_ASM_FILTERED)
  _OBJ   := $$(_ORG_OBJ)
  _FLAGS := $$(filter -Wa%,$$(_ORG_FLAGS)) -c
  _TEXT  := "Assembly     "
  $$(eval $$(call ev-build-file))
endif
endef

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

_FLAGS := $$($$(my)CFLAGS) \
          $$(call get-src-file-target-cflags,$(1)) \
          $$(call host-c-includes,$$(LOCAL_C_INCLUDES) $$(LOCAL_PATH)) \
          $$(LOCAL_CFLAGS) \
          $$(NDK_APP_CFLAGS) \
          $$(call host-c-includes,$$($(my)C_INCLUDES)) \
          -c \

_TEXT := "Compile $$(call get-src-file-text,$1)"
_CC   := $$(NDK_CCACHE) $$(TARGET_CC)

$$(eval $$(call ev-build-source-file))
endef

# -----------------------------------------------------------------------------
# Function  : compile-c-source
# Arguments : 1: single C source file name (relative to LOCAL_PATH)
#             2: object file
# Returns   : None
# Usage     : $(call compile-c-source,<srcfile>,<objfile>)
# Rationale : Setup everything required to build a single C source file
# -----------------------------------------------------------------------------
compile-c-source = $(eval $(call ev-compile-c-source,$1,$2))

# -----------------------------------------------------------------------------
# Function  : compile-s-source
# Arguments : 1: single Assembly source file name (relative to LOCAL_PATH)
#             2: object file
# Returns   : None
# Usage     : $(call compile-s-source,<srcfile>,<objfile>)
# Rationale : Setup everything required to build a single Assembly source file
# -----------------------------------------------------------------------------
compile-s-source = $(eval $(call ev-compile-c-source,$1,$2))


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
_FLAGS := $$($$(my)CXXFLAGS) \
          $$(call get-src-file-target-cflags,$(1)) \
          $$(call host-c-includes, $$(LOCAL_C_INCLUDES) $$(LOCAL_PATH)) \
          $$(LOCAL_CFLAGS) \
          $$(LOCAL_CPPFLAGS) \
          $$(LOCAL_CXXFLAGS) \
          $$(NDK_APP_CFLAGS) \
          $$(NDK_APP_CPPFLAGS) \
          $$(NDK_APP_CXXFLAGS) \
          $$(call host-c-includes,$$($(my)C_INCLUDES)) \
          -c \

_CC   := $$(NDK_CCACHE) $$($$(my)CXX)
_TEXT := "Compile++ $$(call get-src-file-text,$1)"

$$(eval $$(call ev-build-source-file))
endef

# -----------------------------------------------------------------------------
# Function  : compile-cpp-source
# Arguments : 1: single C++ source file name (relative to LOCAL_PATH)
#           : 2: object file name
# Returns   : None
# Usage     : $(call compile-c-source,<srcfile>)
# Rationale : Setup everything required to build a single C++ source file
# -----------------------------------------------------------------------------
compile-cpp-source = $(eval $(call ev-compile-cpp-source,$1,$2))

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


#
#  Module imports
#

# Initialize import list
import-init = $(eval __ndk_import_dirs :=)

# Add an optional single directory to the list of import paths
#
import-add-path-optional = \
  $(if $(strip $(wildcard $1)),\
    $(call ndk_log,Adding import directory: $1)\
    $(eval __ndk_import_dirs += $1)\
  )\

# Add a directory to the list of import paths
# This will warn if the directory does not exist
#
import-add-path = \
  $(if $(strip $(wildcard $1)),\
    $(call ndk_log,Adding import directory: $1)\
    $(eval __ndk_import_dirs += $1)\
  ,\
    $(call __ndk_info,WARNING: Ignoring unknown import directory: $1)\
  )\

import-find-module = $(strip \
      $(eval __imported_module :=)\
      $(foreach __import_dir,$(__ndk_import_dirs),\
        $(if $(__imported_module),,\
          $(call ndk_log,  Probing $(__import_dir)/$1/Android.mk)\
          $(if $(strip $(wildcard $(__import_dir)/$1/Android.mk)),\
            $(eval __imported_module := $(__import_dir)/$1)\
          )\
        )\
      )\
      $(__imported_module)\
    )

# described in docs/IMPORT-MODULE.TXT
# $1: tag name for the lookup
#
# Small technical note on __ndk_import_depth: we use this variable to
# record the depth of recursive import-module calls. The variable is
# initially empty, and we append a "x" to it each time import-module is
# called. I.e. for three recursive calls to import-module, we would get
# the values:
#
#   first call:   x
#   second call:  xx
#   third call:   xxx
#
# This is used in module-add to add the top-level modules (i.e. those
# that are not added with import-module) to __ndk_top_modules, corresponding
# to the default list of wanted modules (see setup-toolchain.mk).
#
import-module = \
    $(eval __import_tag := $(strip $1))\
    $(if $(call seq,$(words $(__import_tag)),1),,\
      $(call __ndk_info,$(call local-makefile): Cannot import module with spaces in tag: '$(__import_tag)')\
    )\
    $(if $(call set_is_member,$(__ndk_import_list),$(__import_tag)),\
      $(call ndk_log,Skipping duplicate import for module with tag '$(__import_tag)')\
    ,\
      $(call ndk_log,Looking for imported module with tag '$(__import_tag)')\
      $(eval __imported_path := $(call import-find-module,$(__import_tag)))\
      $(if $(__imported_path),\
        $(call ndk_log,    Found in $(__imported_path))\
        $(eval __ndk_import_depth := $(__ndk_import_depth)x) \
        $(eval __ndk_import_list := $(call set_insert,$(__ndk_import_list),$(__import_tag)))\
        $(eval include $(__imported_path)/Android.mk)\
        $(eval __ndk_import_depth := $(__ndk_import_depth:%x=%))\
      ,\
        $(call __ndk_info,$(call local-makefile): Cannot find module with tag '$(__import_tag)' in import path)\
        $(call __ndk_info,Are you sure your NDK_MODULE_PATH variable is properly defined ?)\
        $(call __ndk_info,The following directories were searched:)\
        $(for __import_dir,$(__ndk_import_dirs),\
          $(call __ndk_info,    $(__import_dir))\
        )\
        $(call __ndk_error,Aborting.)\
      )\
   )

# Only used for debugging
#
import-debug = \
    $(info IMPORT DIRECTORIES:)\
    $(foreach __dir,$(__ndk_import_dirs),\
      $(info -- $(__dir))\
    )\

#
#  Module classes
#
NDK_MODULE_CLASSES :=

# Register a new module class
# $1: class name (e.g. STATIC_LIBRARY)
# $2: optional file prefix (e.g. 'lib')
# $3: optional file suffix (e.g. '.so')
#
module-class-register = \
    $(eval NDK_MODULE_CLASSES += $1) \
    $(eval NDK_MODULE_CLASS.$1.FILE_PREFIX := $2) \
    $(eval NDK_MODULE_CLASS.$1.FILE_SUFFIX := $3) \
    $(eval NDK_MODULE_CLASS.$1.INSTALLABLE := $(false)) \

# Same a module-class-register, for installable modules
#
# An installable module is one that will be copied to $PROJECT/libs/<abi>/
# during the NDK build.
#
# $1: class name
# $2: optional file prefix
# $3: optional file suffix
#
module-class-register-installable = \
    $(call module-class-register,$1,$2,$3) \
    $(eval NDK_MODULE_CLASS.$1.INSTALLABLE := $(true))

module-class-set-prebuilt = \
    $(eval NDK_MODULE_CLASS.$1.PREBUILT := $(true))

# Returns $(true) if $1 is a valid/registered LOCAL_MODULE_CLASS value
#
module-class-check = $(call set_is_member,$(NDK_MODULE_CLASSES),$1)

# Returns $(true) if $1 corresponds to an installable module class
#
module-class-is-installable = $(if $(NDK_MODULE_CLASS.$1.INSTALLABLE),$(true),$(false))

# Returns $(true) if $1 corresponds to an installable module class
#
module-class-is-prebuilt = $(if $(NDK_MODULE_CLASS.$1.PREBUILT),$(true),$(false))

#
# Register valid module classes
#

# static libraries:
# <foo> -> lib<foo>.a by default
$(call module-class-register,STATIC_LIBRARY,lib,.a)

# shared libraries:
# <foo> -> lib<foo>.so
# a shared library is installable.
$(call module-class-register-installable,SHARED_LIBRARY,lib,.so)

# executable
# <foo> -> <foo>
# an executable is installable.
$(call module-class-register-installable,EXECUTABLE,,)

# prebuilt shared library
# <foo> -> <foo>  (we assume it is already well-named)
# it is installable
$(call module-class-register-installable,PREBUILT_SHARED_LIBRARY,,)
$(call module-class-set-prebuilt,PREBUILT_SHARED_LIBRARY)

# prebuilt static library
# <foo> -> <foo> (we assume it is already well-named)
$(call module-class-register,PREBUILT_STATIC_LIBRARY,,)
$(call module-class-set-prebuilt,PREBUILT_STATIC_LIBRARY)

#
# C++ STL support
#

# The list of registered STL implementations we support
NDK_STL_LIST :=

# Used internally to register a given STL implementation, see below.
#
# $1: STL name as it appears in APP_STL (e.g. system)
# $2: STL module name (e.g. cxx-stl/system)
# $3: list of static libraries all modules will depend on
# $4: list of shared libraries all modules will depend on
#
ndk-stl-register = \
    $(eval __ndk_stl := $(strip $1)) \
    $(eval NDK_STL_LIST += $(__ndk_stl)) \
    $(eval NDK_STL.$(__ndk_stl).IMPORT_MODULE := $(strip $2)) \
    $(eval NDK_STL.$(__ndk_stl).STATIC_LIBS := $(strip $3)) \
    $(eval NDK_STL.$(__ndk_stl).SHARED_LIBS := $(strip $4))

# Called to check that the value of APP_STL is a valid one.
# $1: STL name as it apperas in APP_STL (e.g. 'system')
#
ndk-stl-check = \
    $(if $(call set_is_member,$(NDK_STL_LIST),$1),,\
        $(call __ndk_info,Invalid APP_STL value: $1)\
        $(call __ndk_info,Please use one of the following instead: $(NDK_STL_LIST))\
        $(call __ndk_error,Aborting))

# Called before the top-level Android.mk is parsed to
# select the STL implementation.
# $1: STL name as it appears in APP_STL (e.g. system)
#
ndk-stl-select = \
    $(call import-module,$(NDK_STL.$1.IMPORT_MODULE))

# Called after all Android.mk files are parsed to add
# proper STL dependencies to every C++ module.
# $1: STL name as it appears in APP_STL (e.g. system)
#
ndk-stl-add-dependencies = \
    $(call modules-add-c++-dependencies,\
        $(NDK_STL.$1.STATIC_LIBS),\
        $(NDK_STL.$1.SHARED_LIBS))

#
#

# Register the 'system' STL implementation
#
$(call ndk-stl-register,\
    system,\
    cxx-stl/system,\
    libstdc++,\
    )

# Register the 'stlport_static' STL implementation
#
$(call ndk-stl-register,\
    stlport_static,\
    cxx-stl/stlport,\
    stlport_static,\
    )

# Register the 'stlport_shared' STL implementation
#
$(call ndk-stl-register,\
    stlport_shared,\
    cxx-stl/stlport,\
    ,\
    stlport_shared\
    )

# Register the 'gnustl_static' STL implementation
#
$(call ndk-stl-register,\
    gnustl_static,\
    cxx-stl/gnu-libstdc++,\
    gnustl_static,\
    \
    )

$(call ndk-stl-register,\
    gnustl_shared,\
    cxx-stl/gnu-libstdc++,\
    ,\
    gnustl_shared\
    )

# Register the static version of the GAbi++ C++ runtime
#
$(call ndk-stl-register,\
    gabi++_static,\
    cxx-stl/gabi++,\
    gabi++_static,\
    )

# Register the shared version of the GAbi++ C++ runtime
#
$(call ndk-stl-register,\
    gabi++_shared,\
    cxx-stl/gabi++,\
    gabi++_shared,\
    )

# The 'none' APP_STL value corresponds to no C++ support at
# all. Used by some of the STLport and GAbi++ test projects.
#
$(call ndk-stl-register,\
    none,\
    cxx-stl/system,\
    )
