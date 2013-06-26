# Copyright (C) 2013 The Android Open Source Project
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

# This is a simple GNU Makefile that can be used to rebuild very simple
# host NDK executables. To use from your own GNUmakefile, do the following:
#
# 1/ Define the following variables:
#
#      SOURCES: A list of source files, relative your current GNUmakefile
#               location. Supported extensions as '.c' and '.cc'
#
#      INCLUDES: A list of directories to add to the include path.
#
#      PROGNAME: Program name
#
# 2/ Include this file with 'include .../path/to/<this-file>"
#
# When running make, the following extra parameters are supported:
#
#    MINGW=true  -> build a Windows executable with mingw-w64 on Linux.
#    DEBUG=true  -> build a debug executable, instead of a release one.
#    STATIC=true -> build a static executable.
#
# And the following variables can also be overriden, if you want to
# avoid auto-detection.
#
#    CC  -> C compiler.
#    CXX -> C++ compiler
#    STRIP -> strip command
#    BUILD_DIR -> build directory.
#    EXECUTABLE -> location and name of final executable.

# Sanity checks
ifeq (,$(strip $(SOURCES)))
$(error Please define SOURCES before including this Makefile)
endif

ifeq (,$(strip $(PROGNAME)))
PROGNAME := $(notdir $(EXECUTABLE))
ifeq (,$(strip $(PROGNAME)))
$(error Please define PROGNAME or EXECUTABLE before including this Makefile)
endif
endif

# The following variables can be overridden by the caller
CC         := gcc
CXX        := g++
STRIP      := strip
BUILD_DIR  := /tmp/ndk-$(USER)/build/build-$(PROGRAM)
EXECUTABLE := /tmp/ndk-$(USER)/$(PROGNAME)

# The rest should be left alone

# Statically link against libstdc++ to make the programs
# easier to distribute.
EXTRA_LDFLAGS := -static-libstdc++ -static-libgcc

ifneq (,$(MINGW))
CC := x86_64-w64-mingw32-gcc
CXX := x86_64-w64-mingw32-g++
EXECUTABLE := $(EXECUTABLE).exe
endif

all: $(EXECUTABLE)

EXTRA_CFLAGS := -Wall -Werror -fno-exceptions -fno-rtti
EXTRA_CFLAGS += $(foreach inc,$(INCLUDES),-I$(inc))

ifneq (,$(strip $(DEBUG)))
  CFLAGS += -O0 -g
  hide = @
  strip-cmd =
else
  CFLAGS += -O2 -s -DNDEBUG
  hide =
  strip-cmd = $(STRIP) $1
  EXTRA_CFLAGS +=  -ffunction-sections -fdata-sections
  EXTRA_LDFLAGS += -Wl,-gc-sections
endif

# Build static executables to make them easier to distribute.
ifneq (,$(STATIC))
EXTRA_CFLAGS += -fno-pic
EXTRA_LDFLAGS += -static
endif


OBJECTS=

c-to-obj = $(subst ..,__,$(patsubst %.c,%.o,$1))
cc-to-obj = $(subst ..,__,$(patsubst %.cc,%.o,$1))
cpp-to-obj = $(subst ..,__,$(patsubst %.cpp,%.o,$1))

define build-c-object
OBJECTS += $1
$1: $2
	mkdir -p $$(dir $1)
	$$(CC) $$(CFLAGS) $$(EXTRA_CFLAGS) -c -o $1 $2
endef

define build-cxx-object
OBJECTS += $1
$1: $2
	mkdir -p $$(dir $1)
	$$(CXX) $$(CFLAGS) $$(EXTRA_CFLAGS) -c -o $1 $2
endef

$(foreach src,$(filter %.c,$(SOURCES)),\
  $(eval $(call build-c-object,$(BUILD_DIR)/$(call c-to-obj,$(src)),$(src)))\
)

$(foreach src,$(filter %.cc,$(SOURCES)),\
  $(eval $(call build-cxx-object,$(BUILD_DIR)/$(call cc-to-obj,$(src)),$(src)))\
)

$(foreach src,$(filter %.cpp,$(SOURCES)),\
  $(eval $(call build-cxx-object,$(BUILD_DIR)/$(call cpp-to-obj,$(src)),$(src)))\
)

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@ $(EXTRA_LDFLAGS)
	$(call strip-cmd,$@)
