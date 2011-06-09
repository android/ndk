/* Copyright (C) 2007-2010 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/*
 * Contains declarations of types, constants and structures
 * describing ELF file format.
 */

#ifndef ELFF_ELH_H_
#define ELFF_ELH_H_

#include <stdint.h>
#include "elff-common.h"

//=============================================================================
// ELF file definitions
//=============================================================================

/*
 * ELF format documentation uses Elf##_Xxx notation for data types, where
 * ## stands for CPU architecture (32, or 64 bit), and Xxx stands for a
 * specific type. For the sake of compliance, we will follow doc's notation
 * when defining types used in ELF file descriptors. However, for the sake of
 * code simplicity, we will drop CPU architecture index from the types that
 * have equal sizes on both, 32 and 64 bit architectures.
 */

/*
 * Architecture independent types.
 */

typedef uint8_t   Elf_Byte;
typedef int8_t    Elf_Sbyte;

typedef uint16_t  Elf_Half;
typedef int16_t   Elf_Shalf;

typedef uint32_t  Elf_Word;
typedef int32_t   Elf_Sword;

typedef uint64_t  Elf_Xword;
typedef int64_t   Elf_Sxword;

/*
 * Architecture dependent types.
 */

/* 32-bit ELF address. */
typedef uint32_t  Elf32_Addr;
/* 32-bit ELF offset. */
typedef uint32_t  Elf32_Off;

/* 64-bit ELF address. */
typedef uint64_t  Elf64_Addr;
/* 64-bit ELF offset. */
typedef uint64_t  Elf64_Off;

//=============================================================================
// ELF file header
//=============================================================================

/* Byte size of the fixed portion of ELF header. */
#define EI_NIDENT	16

/* Common (architecture independent portion of) ELF file header,
 * that starts at offset 0 in ELF file.
 */
typedef struct Elf_CommonHdr {
  union {
    struct {
      /* ei_mag0 - ei_mag3 contain ELF header signature. See ELFMAGx bellow. */
      Elf_Byte  ei_mag0;
      Elf_Byte  ei_mag1;
      Elf_Byte  ei_mag2;
      Elf_Byte  ei_mag3;

      /* File class (32, or 64 bits). See ELFCLASSxxx bellow. */
      Elf_Byte  ei_class;

      /* Data encoding (endianness). See ELFDATAxxx bellow. */
      Elf_Byte  ei_data;

      /* ELF header version number. */
      Elf_Byte  ei_version;
    } ei_info;
    unsigned char e_ident[EI_NIDENT];
  };

  /* File type (executable, shared object, etc.) */
  Elf_Half      e_type;

  /* Processor type. */
  Elf_Half      e_machine;

  /* File version. */
  Elf_Word      e_version;
} Elf_CommonHdr;


/* ELF header signature. */
#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'
#define ELFMAG		"\177ELF"
#define SELFMAG		4

/*
 * Possible ei_class values.
 */

/* Invalid. */
#define ELFCLASSNONE  0
/* It's 32-bit ELF file. */
#define ELFCLASS32    1
/* It's 64-bit ELF file. */
#define ELFCLASS64    2

/*
 * Possible ei_data values.
 */

/* Invalid. */
#define ELFDATANONE   0
/* ELF data is formatted in little-endian. */
#define ELFDATA2LSB   1
/* ELF data is formatted in big-endian. */
#define ELFDATA2MSB   2

/* Tempated (architecture dependent) ELF file header.
 * Template param:
 *  Elf_Addr - Actual type for address encoding (Elf32_Addr, or Elf64_Addr).
 *  Elf_Off - Actual type for offset encoding (Elf32_Off, or Elf64_Off).
 */
template <typename Elf_Addr, typename Elf_Off>
struct Elf_FHdr {
  /* Common header. */
  Elf_CommonHdr common;

  /* Module entry point. */
  Elf_Addr      e_entry;

  /* Programm header table offset (in bytes) from the beginning of the file.
   * Zero if there is no programm header in this file.
   */
  Elf_Off       e_phoff;

  /* Section header table offset (in bytes) from the beginning of the file.
   * Zero if there is no section header in this file.
   */
  Elf_Off       e_shoff;

  /* Processor-specific flags. */
  Elf_Word      e_flags;

  /* This header size in bytes. */
  Elf_Half      e_ehsize;

  /* Byte size of an entry in programm header table. All entries
   * in the table are the same size.
   */
  Elf_Half      e_phentsize;

  /* Number of entries in programm header table. */
  Elf_Half      e_phnum;

  /* Byte size of an entry in section header table. All entries
   * in the table are the same size.
   */
  Elf_Half      e_shentsize;

  /* Number of entries in section header table. */
  Elf_Half      e_shnum;

  /* Zero-based index of an entry for name string table section in the section
   * header table. If no such section exists in the file this field contains
   * SHN_UNDEF value.
   */
  Elf_Half      e_shstrndx;
};
/* 32-bit ELF header. */
typedef Elf_FHdr<Elf32_Addr, Elf32_Off> Elf32_FHdr;
/* 64-bit ELF header. */
typedef Elf_FHdr<Elf64_Addr, Elf64_Off> Elf64_FHdr;

//=============================================================================
// ELF section header
//=============================================================================

/* Templated (architecture dependent) section header for ELF file.
 * Template param:
 *  Elf_Addr - Actual type for address encoding (Elf32_Addr, or Elf64_Addr).
 *  Elf_Off - Actual type for offset encoding (Elf32_Off, or Elf64_Off).
 */
template <typename Elf_Addr, typename Elf_Off>
struct Elf_SHdr {
  /* Index (byte offset) of section name in the name string table section. */
  Elf_Word    sh_name;

  /* Section type and semantics. */
  Elf_Word    sh_type;

  /* Section flags and attributes. */
  Elf_Word    sh_flags;

  /* Section address in the memory image of the process. */
  Elf_Addr    sh_addr;

  /* Byte offset from the beginning of the ELF file to the first
   * byte in the section.
   */
  Elf_Off     sh_offset;

  /* Section size in bytes. */
  Elf_Word    sh_size;

  /* Section header table index link. Depends on section type. */
  Elf_Word    sh_link;

  /* Extra section information, depending on the section type. */
  Elf_Word    sh_info;

  /* Address alignment constrains. 0 and 1 means that section has no
   * alignment constrains.
   */
  Elf_Word    sh_addralign;

  /* Entry size for sections that hold some kind of a table. */
  Elf_Word    sh_entsize;
};
/* 32-bit section header. */
typedef Elf_SHdr<Elf32_Addr, Elf32_Off> Elf32_SHdr;
/* 64-bit section header. */
typedef Elf_SHdr<Elf64_Addr, Elf64_Off> Elf64_SHdr;

/*
 * Special section indices
 */
#define SHN_UNDEF       0
#define SHN_LORESERVE   0xff00
#define SHN_LOPROC      0xff00
#define SHN_HIPROC      0xff1f
#define SHN_LOOS        0xff20
#define SHN_HIOS        0xff3f
#define SHN_ABS         0xfff1
#define SHN_COMMON      0xfff2
#define SHN_XINDEX      0xffff
#define SHN_HIRESERVE   0xffff

/*
 * Values for sh_type
 */
#define SHT_NULL            0
#define SHT_PROGBITS        1
#define SHT_SYMTAB          2
#define SHT_STRTAB          3
#define SHT_RELA            4
#define SHT_HASH            5
#define SHT_DYNAMIC         6
#define SHT_NOTE            7
#define SHT_NOBITS          8
#define SHT_REL             9
#define SHT_SHLIB           10
#define SHT_DYNSYM          11
#define SHT_INIT_ARRAY      14
#define SHT_FINI_ARRAY      15
#define SHT_PREINIT_ARRAY   16
#define SHT_GROUP           17
#define SHT_SYMTAB_SHNDX    18
#define SHT_NUM             19

#endif  // ELFF_ELH_H_
