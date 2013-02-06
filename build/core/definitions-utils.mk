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

-testfind-first.empty = \
    $(eval -local-pred = $$(call seq,foo,$$1))\
    $(call test-expect,,$(call find-first,,-local-pred))\
    $(call test-expect,,$(call find-first,bar,-local-pred))

-testfind-first.simple = \
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


###########################################################################
# Internal function used by modules-get-closure
# Compute the closure of a node in a graph.
# $1: list of graph nodes
# $2: dependency function, i.e. $(call $2,<name>) should return the list
#     of nodes that <name> depends on.
# Out: list of nodes. This are all the nodes that depend on those in $1
#      transitively.
#
get-closure = $(strip \
    $(eval __closure_list := $1) \
    $(eval __closure_wq   := $1) \
    $(eval __closure_deps_func := $2) \
    $(call get-closure-recursive)\
    $(__closure_list))

# Note the tricky use of conditional recursion to work around the fact that
# the GNU Make language does not have any conditional looping construct
# like 'while'.
get-closure-recursive = \
    $(eval __closure_node := $(call first,$(__closure_wq)))\
    $(eval __closure_wq   := $(call rest,$(__closure_wq)))\
    $(eval __closure_depends := $(call $(__closure_deps_func),$(__closure_node)))\
    $(eval __closure_new := $(filter-out $(__closure_list),$(__closure_depends)))\
    $(eval __closure_list += $(__closure_new))\
    $(eval __closure_wq := $(strip $(__closure_wq) $(__closure_new)))\
    $(if $(__closure_wq),$(call get-closure-recursive))

-test-get-closure.empty = \
    $(eval deps = $$($$1_depends))\
    $(call test-expect,$(call get-closure,,deps))\

-test-get-closure.A = \
    $(eval deps = $$($$1_depends))\
    $(eval A_depends :=)\
    $(call test-expect,A,$(call get-closure,A,deps))

-test-get-closure.ABC = \
    $(eval deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends :=)\
    $(call test-expect,A B C,$(call get-closure,A,deps))

-test-get-closure.ABC_circular = \
    $(eval deps = $$($$1_depends))\
    $(eval A_depends := B)\
    $(eval B_depends := C)\
    $(eval C_depends := A)\
    $(call test-expect,A B C,$(call get-closure,A,deps))

-test-get-closure.ABCDEF = \
    $(eval deps = $$($$1_depends))\
    $(eval A_depends := B C)\
    $(eval B_depends := D E)\
    $(eval C_depends := E F)\
    $(eval D_depends :=)\
    $(eval E_depends :=)\
    $(eval F_depends :=)\
    $(call test-expect,A B C D E F,$(call get-closure,A,deps))

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

