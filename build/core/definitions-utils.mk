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

# Common utility functions.
#
# NOTE: All the functions here should be purely functional, i.e. avoid
#       using global variables or depend on the file system / environment
#       variables. This makes testing easier.

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

-test-remove-duplicates = \
  $(call test-expect,,$(call remove-duplicates))\
  $(call test-expect,foo bar,$(call remove-duplicates,foo bar))\
  $(call test-expect,foo bar,$(call remove-duplicates,foo bar foo bar))\
  $(call test-expect,foo bar,$(call remove-duplicates,foo foo bar bar bar))

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
# Function : filter-by
# Arguments: 1: list
#            2: predicate function, will be called as $(call $2,<name>)
#               and it this returns a non-empty value, then <name>
#               will be appended to the result.
# Returns  : elements of $1 that satisfy the predicate function $2
# -----------------------------------------------------------------------------
filter-by = $(strip \
  $(foreach __filter_by_n,$1,\
    $(if $(call $2,$(__filter_by_n)),$(__filter_by_n))))

-test-filter-by = \
    $(eval -local-func = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call filter-by,,-local-func))\
    $(call test-expect,foo,$(call filter-by,foo,-local-func))\
    $(call test-expect,foo,$(call filter-by,foo bar,-local-func))\
    $(call test-expect,foo foo,$(call filter-by,aaa foo bar foo,-local-func))\
    $(eval -local-func = $$(call sne,foo,$$1))\
    $(call test-expect,,$(call filter-by,,-local-func))\
    $(call test-expect,,$(call filter-by,foo,-local-func))\
    $(call test-expect,bar,$(call filter-by,foo bar,-local-func))\
    $(call test-expect,aaa bar,$(call filter-by,aaa foo bar,-local-func))

# -----------------------------------------------------------------------------
# Function : filter-out-by
# Arguments: 1: list
#            2: predicate function, will be called as $(call $2,<name>)
#               and it this returns an empty value, then <name>
#               will be appended to the result.
# Returns  : elements of $1 that do not satisfy the predicate function $2
# -----------------------------------------------------------------------------
filter-out-by = $(strip \
  $(foreach __filter_out_by_n,$1,\
    $(if $(call $2,$(__filter_out_by_n)),,$(__filter_out_by_n))))

-test-filter-out-by = \
    $(eval -local-func = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call filter-out-by,,-local-func))\
    $(call test-expect,,$(call filter-out-by,foo,-local-func))\
    $(call test-expect,bar,$(call filter-out-by,foo bar,-local-func))\
    $(call test-expect,aaa bar,$(call filter-out-by,aaa foo bar foo,-local-func))\
    $(eval -local-func = $$(call sne,foo,$$1))\
    $(call test-expect,,$(call filter-out-by,,-local-func))\
    $(call test-expect,foo,$(call filter-out-by,foo,-local-func))\
    $(call test-expect,foo,$(call filter-out-by,foo bar,-local-func))\
    $(call test-expect,foo foo,$(call filter-out-by,aaa foo bar foo,-local-func))

# -----------------------------------------------------------------------------
# Function : find-first
# Arguments: 1: list
#            2: predicate function, will be called as $(call $2,<name>).
# Returns  : the first item of $1 that satisfies the predicate.
# -----------------------------------------------------------------------------
find-first = $(firstword $(call filter-by,$1,$2))

-test-find-first.empty = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call find-first,,-local-pred))\
    $(call test-expect,,$(call find-first,bar,-local-pred))

-test-find-first.simple = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,foo,$(call find-first,foo,-local-pred))\
    $(call test-expect,foo,$(call find-first,aaa foo bar,-local-pred))\
    $(call test-expect,foo,$(call find-first,aaa foo foo bar,-local-pred))

# -----------------------------------------------------------------------------
# Function : parent-dir
# Arguments: 1: path
# Returns  : Parent dir or path of $1, with final separator removed.
# -----------------------------------------------------------------------------
parent-dir = $(patsubst %/,%,$(dir $(1:%/=%)))

-test-parent-dir = \
  $(call test-expect,,$(call parent-dir))\
  $(call test-expect,.,$(call parent-dir,foo))\
  $(call test-expect,foo,$(call parent-dir,foo/bar))\
  $(call test-expect,foo,$(call parent-dir,foo/bar/))

# -----------------------------------------------------------------------------
# Strip any 'lib' prefix in front of a given string.
#
# Function : strip-lib-prefix
# Arguments: 1: module name
# Returns  : module name, without any 'lib' prefix if any
# Usage    : $(call strip-lib-prefix,$(LOCAL_MODULE))
# -----------------------------------------------------------------------------
strip-lib-prefix = $(1:lib%=%)

-test-strip-lib-prefix = \
  $(call test-expect,,$(call strip-lib-prefix,))\
  $(call test-expect,foo,$(call strip-lib-prefix,foo))\
  $(call test-expect,foo,$(call strip-lib-prefix,libfoo))\
  $(call test-expect,nolibfoo,$(call strip-lib-prefix,nolibfoo))\
  $(call test-expect,foolib,$(call strip-lib-prefix,foolib))\
  $(call test-expect,foo bar,$(call strip-lib-prefix,libfoo libbar))

