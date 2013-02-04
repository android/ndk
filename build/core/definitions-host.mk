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

# These definitions contain a few host-specific functions. I.e. they are
# typically used to generate shell commands during the build and their
# implementation will depend on the value of the HOST_OS variable.
#

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
# Function : host-rm
# Arguments: 1: list of files
# Usage    : $(call host-rm,<files>)
# Rationale: This function expands to the host-specific shell command used
#            to remove some files.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-rm = \
    $(eval __host_rm_files := $(foreach __host_rm_file,$1,$(subst /,\,$(wildcard $(__host_rm_file)))))\
    $(if $(__host_rm_files),del /f/q $(__host_rm_files) >NUL 2>NUL)
else
host-rm = rm -f $1
endif

# -----------------------------------------------------------------------------
# Function : host-rmdir
# Arguments: 1: list of files or directories
# Usage    : $(call host-rm,<files>)
# Rationale: This function expands to the host-specific shell command used
#            to remove some files _and_ directories.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-rmdir = \
    $(eval __host_rmdir_files := $(foreach __host_rmdir_file,$1,$(subst /,\,$(wildcard $(__host_rmdir_file)))))\
    $(if $(__host_rmdir_files),del /f/s/q $(__host_rmdir_files) >NUL 2>NUL)
else
host-rmdir = rm -rf $1
endif

# -----------------------------------------------------------------------------
# Function : host-mkdir
# Arguments: 1: directory path
# Usage    : $(call host-mkdir,<path>
# Rationale: This function expands to the host-specific shell command used
#            to create a path if it doesn't exist.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-mkdir = md $(subst /,\,"$1") >NUL 2>NUL || rem
else
host-mkdir = mkdir -p $1
endif

# -----------------------------------------------------------------------------
# Function : host-cp
# Arguments: 1: source file
#            2: target file
# Usage    : $(call host-cp,<src-file>,<dst-file>)
# Rationale: This function expands to the host-specific shell command used
#            to copy a single file
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-cp = copy /b/y $(subst /,\,"$1" "$2") > NUL
else
host-cp = cp -f $1 $2
endif

# -----------------------------------------------------------------------------
# Function : host-install
# Arguments: 1: source file
#            2: target file
# Usage    : $(call host-install,<src-file>,<dst-file>)
# Rationale: This function expands to the host-specific shell command used
#            to install a file or directory, while preserving its timestamps
#            (if possible).
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-install = copy /b/y $(subst /,\,"$1" "$2") > NUL
else
host-install = install -p $1 $2
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
# Function : host-copy-if-differ
# Arguments: 1: source file
#            2: destination file
# Usage    : $(call host-copy-if-differ,<src-file>,<dst-file>)
# Rationale: This function copy source file to destination file if contents are
#            different.
# -----------------------------------------------------------------------------
ifeq ($(HOST_OS),windows)
host-copy-if-differ = $(HOST_CMP) -s $1 $2 > NUL || copy /b/y $(subst /,\,"$1" "$2") > NUL
else
host-copy-if-differ = $(HOST_CMP) -s $1 $2 > /dev/null 2>&1 || cp -f $1 $2
endif

