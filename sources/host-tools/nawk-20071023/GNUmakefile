# Custom GNUMakefile to rebuild nawk for the Android NDK
# (C) 2011 The Android Open Source Project
# yada yda yada Apache 2.0 License....

BUILD_DIR ?= /tmp/ndk-$(USER)/build-awk


CC       := $(HOST_CC)
CFLAGS   := $(CFLAGS) -O2 -I$(BUILD_DIR) -I.
LDFLAGS  := $(LDFLAGS) -Wl,-s
BISON    := bison

# NATIVE_CC is to build maketab which needs to run in order
# to generate proctab.c from ytab.h.  Set it to host native
# cc if host can run windows binary (MINGW=yes) or darwin binary
# (DARWIN=yes)
NATIVE_CC       ?= $(CC)
NATIVE_CFLAGS   ?= $(CFLAGS)
NATIVE_LDFLAGS  ?= $(LDFLAGS)

MINGW := $(strip $(WIN32))
TRY64 := $(strip $(TRY64))

ifeq ($(V),1)
hide :=
else
hide := @
endif

SOURCES := \
    lex.c \
    b.c \
    main.c \
    parse.c \
    lib.c \
    run.c \
    tran.c \

EXE :=

ifdef MINGW
ifdef TRY64
CC      := x86_64-pc-mingw32msvc-gcc
else
CC      := i586-pc-mingw32msvc-gcc
endif
EXE     := .exe
endif

PROGRAM := $(BUILD_DIR)/ndk-awk$(EXE)

all: $(PROGRAM)

OBJECTS :=

YTAB_H := $(BUILD_DIR)/ytab.h
YTAB_C := $(BUILD_DIR)/ytab.c
YTAB_O := $(YTAB_C:.c=.o)
$(YTAB_C): awkgram.y
	@echo "Bison: ytab.c"
	$(hide)$(BISON) -o $@ --defines=$(YTAB_H) awkgram.y

$(YTAB_H): $(YTAB_C) awkgram.y

OBJECTS += $(YTAB_O)

# $1: source file
# $2: object file
define build-c-file
OBJECTS += $2
$2: $1 $(YTAB_H)
	@echo "Compile: $1"
	$$(hide)mkdir -p $$(dir $$@)
	$$(hide)$$(CC) $$(CFLAGS) -c -o $$@ $$<
endef

$(foreach src,$(filter %.c,$(SOURCES)),\
    $(eval $(call build-c-file,$(src),$(BUILD_DIR)/$(src:%.c=%.o)))\
)

MAKETAB := $(BUILD_DIR)/maketab$(EXE)
$(MAKETAB): maketab.c $(BUILD_DIR)/ytab.h
	@echo "Native Executable: $(notdir $@)"
	$(hide)$(NATIVE_CC) $(NATIVE_CFLAGS) $(NATIVE_LDFLAGS) maketab.c -o $@

PROCTAB_C := $(BUILD_DIR)/proctab.c
PROCTAB_O := $(PROCTAB_C:%.c=%.o)

$(eval $(call build-c-file,$(PROCTAB_C),$(PROCTAB_O)))

$(PROCTAB_C): $(MAKETAB) $(YTAB_H)
	@echo "Gen: $(notdir $@)"
	$(hide)$(MAKETAB) $(YTAB_H) > $@


$(PROGRAM): $(OBJECTS)
	@echo "Link: $(notdir $@)"
	$(hide)$(CC) $(LDFLAGS) -o $@ $(OBJECTS) -lm

clean:
	$(hide)rm -f $(OBJECTS) $(PROGRAM) $(MAKETAB) $(YTAB_C) $(YTAB_H) $(PROGTAB_C)
