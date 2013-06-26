# Copyright (C) 2011 The Android Open Source Project
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

PROGNAME := ndk-stack

INCLUDES :=

ELFF_SOURCES := elff/dwarf_cu.cc \
                elff/dwarf_die.cc \
                elff/dwarf_utils.cc \
                elff/elf_alloc.cc \
                elff/elf_file.cc \
                elff/elf_mapped_section.cc \
                elff/elff_api.cc \
                elff/mapfile.c

REGEX_SOURCES := regex/regcomp.c \
                 regex/regerror.c \
                 regex/regexec.c \
                 regex/regfree.c

NDK_STACK_SOURCES := ndk-stack.c \
                     ndk-stack-parser.c

SOURCES := $(NDK_STACK_SOURCES) $(ELFF_SOURCES) $(REGEX_SOURCES)

include ../ndk-common/build-host-executable.mk
