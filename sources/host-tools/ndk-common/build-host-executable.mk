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
#      DEFINES: A list of macro definitions that apply to both C and C++
#               sources.
#
#      CFLAGS: A list of compiler flags that apply to both C and C++
#              sources.
#
#      CXXFLAGS: A list of compiler flags that only apply to C++ sources.
#
#      INCLUDES: A list of directories to add to the include path.
#
#      LINK_LIBS: Optional link-time libraries.
#
#      PROGNAME: Program name
#
# 2/ Include this file with 'include .../path/to/<this-file>"
#
# When running make, the following extra parameters are supported:
#
#    V=1         -> verbose build output.
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
BUILD_DIR  := /tmp/ndk-$(USER)/build/build-$(PROGNAME)
EXECUTABLE := /tmp/ndk-$(USER)/$(PROGNAME)

# The rest should be left alone

hide := @
ifeq (1,$(V))
hide :=
endif

ifneq (,$(MINGW))
CC := x86_64-w64-mingw32-gcc
CXX := x86_64-w64-mingw32-g++
EXECUTABLE := $(EXECUTABLE).exe
endif

# On Windows, always force a static build.
ifneq (,$(filter %.exe,$(EXECUTABLE)))
FORCE_STATIC := true
endif

all: $(EXECUTABLE)

EXTRA_CFLAGS :=
EXTRA_LDFLAGS :=
EXTRA_CFLAGS += $(foreach inc,$(INCLUDES),-I$(inc))
EXTRA_CFLAGS += $(forech def,$(DEFINES),-D$(def))

ifneq (,$(strip $(DEBUG)))
  CFLAGS += -O0 -g
  strip-cmd =
else
  CFLAGS += -O2 -s -DNDEBUG
  strip-cmd = $(STRIP) $1
  EXTRA_CFLAGS +=  -ffunction-sections -fdata-sections
  EXTRA_LDFLAGS += -Wl,-gc-sections
endif

# Statically link against libstdc++ and libgcc to make the programs
# easier to distribute.
ifneq (,$(STATIC)$(FORCE_STATIC))
EXTRA_LDFLAGS += -static-libstdc++ -static-libgcc
endif

OBJECTS=

c-to-obj = $(subst ..,__,$(patsubst %.c,%.o,$1))
cc-to-obj = $(subst ..,__,$(patsubst %.cc,%.o,$1))
cpp-to-obj = $(subst ..,__,$(patsubst %.cpp,%.o,$1))

define build-c-object
OBJECTS += $1
$1: $2
	$(hide) mkdir -p $$(dir $1)
	$(hide) $$(CC) $$(EXTRA_CFLAGS) $$(CFLAGS) $$(EXTRA_CFLAGS) -c -o $1 $2
endef

define build-cxx-object
OBJECTS += $1
$1: $2
	$(hide) mkdir -p $$(dir $1)
	$(hide) $$(CXX) $$(EXTRA_CFLAGS) $$(CFLAGS) $$(CXXFLAGS) -c -o $1 $2
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
	$(hide) rm -f $(EXECUTABLE) $(OBJECTS)

$(EXECUTABLE): $(OBJECTS)
	$(hide) $(CXX) $(LDFLAGS) $(OBJECTS) $(LINK_LIBS) -o $@ $(EXTRA_LDFLAGS)
	$(hide) $(call strip-cmd,$@)
