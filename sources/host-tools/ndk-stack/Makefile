CC=g++
CFLAGS=-c -Wall
LDFLAGS=

INCS = elff \
       regex
DEFS =

ELFF_SOURCES = elff/dwarf_cu.cc \
               elff/dwarf_die.cc \
               elff/dwarf_utils.cc \
               elff/elf_alloc.cc \
               elff/elf_file.cc \
               elff/elf_mapped_section.cc \
               elff/elff_api.cc \
               elff/mapfile.c

REGEX_SOURCES = regex/regcomp.c \
                regex/regerror.c \
                regex/regexec.c \
                regex/regfree.c

NDK_STACK_SOURCES = ndk-stack.c \
                    ndk-stack-parser.c

SOURCES = $(NDK_STACK_SOURCES) $(ELFF_SOURCES) $(REGEX_SOURCES)
OBJECTS=$(SOURCES:.c*=.o)
EXECUTABLE=ndk-stack

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
