# The following definitions are the defaults used by all toolchains.
# This is included in setup-toolchain.mk just before the inclusion
# of the toolchain's specific setup.mk file which can then override
# these definitions.
#

# These flags are used to ensure that a binary doesn't reference undefined
# flags.
TARGET_NO_UNDEFINED_LDFLAGS := -Wl,--no-undefined


# Return the list of object, static libraries and shared libraries as they
# must appear on the final static linker command (order is important).
#
# This can be over-ridden by a specific toolchain. Note that by default
# we always put libgcc _after_ all static libraries and _before_ shared
# libraries. This ensures that any libgcc function used by the final
# executable will be copied into it. Otherwise, it could contain
# symbol references to the same symbols as exported by shared libraries
# and this causes binary compatibility problems when they come from
# system libraries (e.g. libc.so and others).
#
# IMPORTANT: The result must use the host path convention.
#
# $1: object files
# $2: static libraries
# $3: whole static libraries
# $4: shared libraries
#
TARGET-get-linker-objects-and-libraries = \
    $(call host-path, $1) \
    $(call link-whole-archives,$3) \
    $(call host-path, $2 $(PRIVATE_LIBGCC) $4) \


# These flags are used to enfore the NX (no execute) security feature in the
# generated machine code. This adds a special section to the generated shared
# libraries that instruct the Linux kernel to disable code execution from
# the stack and the heap.
TARGET_NO_EXECUTE_CFLAGS  := -Wa,--noexecstack
TARGET_NO_EXECUTE_LDFLAGS := -Wl,-z,noexecstack

# NOTE: Ensure that TARGET_LIBGCC is placed after all private objects
#       and static libraries, but before any other library in the link
#       command line when generating shared libraries and executables.
#
#       This ensures that all libgcc.a functions required by the target
#       will be included into it, instead of relying on what's available
#       on other libraries like libc.so, which may change between system
#       releases due to toolchain or library changes.
#
define cmd-build-shared-library
$(PRIVATE_CXX) \
    -nostdlib -Wl,-soname,$(notdir $@) \
    -Wl,-shared,-Bsymbolic \
    $(call host-path,\
        $(TARGET_CRTBEGIN_SO_O) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    $(call host-path,\
        $(TARGET_CRTEND_SO_O)) \
    -o $(call host-path,$@)
endef

define cmd-build-executable
$(PRIVATE_CXX) \
    -nostdlib -Bdynamic \
    -Wl,-dynamic-linker,/system/bin/linker \
    -Wl,--gc-sections \
    -Wl,-z,nocopyreloc \
    $(call host-path,\
        $(TARGET_CRTBEGIN_DYNAMIC_O) \
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    $(call host-path,\
        $(TARGET_CRTEND_O)) \
    -o $(call host-path,$@)
endef

define cmd-build-static-library
$(PRIVATE_AR) $(call host-path,$@) $(PRIVATE_AR_OBJECTS)
endef

# The strip command is only used for shared libraries and executables.
# It is thus safe to use --strip-unneeded, which is only dangerous
# when applied to static libraries or object files.
cmd-strip = $(PRIVATE_STRIP) --strip-unneeded $(call host-path,$1)

TARGET_LIBGCC = $(shell $(TARGET_CC) -print-libgcc-file-name)
TARGET_LDLIBS := -lc -lm

#
# IMPORTANT: The following definitions must use lazy assignment because
# the value of TOOLCHAIN_PREFIX or TARGET_CFLAGS can be changed later by
# the toolchain's setup.mk script.
#

TARGET_CC       = $(TOOLCHAIN_PREFIX)gcc
TARGET_CFLAGS   =

TARGET_CXX      = $(TOOLCHAIN_PREFIX)g++
TARGET_CXXFLAGS = $(TARGET_CFLAGS) -fno-exceptions -fno-rtti

TARGET_LD       = $(TOOLCHAIN_PREFIX)ld
TARGET_LDFLAGS :=

TARGET_AR       = $(TOOLCHAIN_PREFIX)ar
TARGET_ARFLAGS := crs

TARGET_STRIP    = $(TOOLCHAIN_PREFIX)strip

# Add linker scripts flags for executables and shared libraries
TARGET_LDSCRIPT_X :=
TARGET_LDSCRIPT_XSC :=
