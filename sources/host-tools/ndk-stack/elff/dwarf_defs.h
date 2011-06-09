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
 * describing DWARF format.
 */

#ifndef ELFF_DWARF_DEFS_H_
#define ELFF_DWARF_DEFS_H_

#include "dwarf.h"
#include "elf_defs.h"

/* DWARF structures are packed to 1 byte. */
#define ELFF_PACKED __attribute__ ((packed))

/*
 * Helper types for misc. DWARF variables.
 */

/* Type for DWARF abbreviation number. */
typedef uint32_t  Dwarf_AbbrNum;

/* Type for DWARF tag ID. */
typedef uint16_t  Dwarf_Tag;

/* Type for DWARF attribute ID. */
typedef uint16_t  Dwarf_At;

/* Type for DWARF form ID. */
typedef uint16_t  Dwarf_Form;

/* Type for offset in 32-bit DWARF. */
typedef uint32_t  Dwarf32_Off;

/* Type for offset in 64-bit DWARF. */
typedef uint64_t  Dwarf64_Off;

/* Enumerates types of values, obtained during DWARF attribute decoding. */
typedef enum DwarfValueType {
  /* Undefined */
  DWARF_VALUE_UNKNOWN = 1,

  /* uint8_t */
  DWARF_VALUE_U8,

  /* int8_t */
  DWARF_VALUE_S8,

  /* uint16_t */
  DWARF_VALUE_U16,

  /* int16_t */
  DWARF_VALUE_S16,

  /* uint32_t */
  DWARF_VALUE_U32,

  /* int32_t */
  DWARF_VALUE_S32,

  /* uint64_t */
  DWARF_VALUE_U64,

  /* int64_t */
  DWARF_VALUE_S64,

  /* const char* */
  DWARF_VALUE_STR,

  /* 32-bit address */
  DWARF_VALUE_PTR32,

  /* 64-bit address */
  DWARF_VALUE_PTR64,

  /* Dwarf_Block */
  DWARF_VALUE_BLOCK,
} DwarfValueType;

/* Describes block of data, stored directly in the mapped .debug_info
 * section. This type is used to represent an attribute encoded with
 * DW_FORM_block# form.
 */
typedef struct Dwarf_Block {
  /* Pointer to the block data inside mapped .debug_info section. */
  const void*   block_ptr;

  /* Byte size of the block data. */
  Elf_Word      block_size;
} Dwarf_Block;

/* Describes a value, obtained from the mapped .debug_info section
 * during DWARF attribute decoding.
 */
typedef struct Dwarf_Value {
  /* Unites all possible data types for the value.
   * See DwarfValueType for the list of types.
   */
  union {
    Elf_Byte      u8;
    Elf_Sbyte     s8;
    Elf_Half      u16;
    Elf_Shalf     s16;
    Elf_Word      u32;
    Elf_Sword     s32;
    Elf_Xword     u64;
    Elf_Sxword    s64;
    Elf_Word      ptr32;
    Elf_Xword     ptr64;
    const char*   str;
    Dwarf_Block   block;
  };

  /* Value type (defines which variable in the union abowe
   * contains the value).
   */
  DwarfValueType  type;

  /* Number of bytes that encode this value in .debug_info section
   * of ELF file.
   */
  Elf_Word        encoded_size;
} Dwarf_Value;

/* DWARF's LEB128 data type. LEB128 is defined as:
 * Variable Length Data. "Little Endian Base 128" (LEB128) numbers. LEB128 is
 * a scheme for encoding integers densely that exploits the assumption that
 * most integers are small in magnitude. (This encoding is equally suitable
 * whether the target machine architecture represents data in big-endian or
 * littleendian order. It is "little endian" only in the sense that it avoids
 * using space to represent the "big" end of an unsigned integer, when the big
 * end is all zeroes or sign extension bits).
 *
 * Unsigned LEB128 numbers are encoded as follows: start at the low order end
 * of an unsigned integer and chop it into 7-bit chunks. Place each chunk into
 * the low order 7 bits of a byte. Typically, several of the high order bytes
 * will be zero; discard them. Emit the remaining bytes in a stream, starting
 * with the low order byte; set the high order bit on each byte except the last
 * emitted byte. The high bit of zero on the last byte indicates to the decoder
 * that it has encountered the last byte. The integer zero is a special case,
 * consisting of a single zero byte.
 *
 * The encoding for signed LEB128 numbers is similar, except that the criterion
 * for discarding high order bytes is not whether they are zero, but whether
 * they consist entirely of sign extension bits. Consider the 32-bit integer
 * -2. The three high level bytes of the number are sign extension, thus LEB128
 * would represent it as a single byte containing the low order 7 bits, with
 * the high order bit cleared to indicate the end of the byte stream. Note that
 * there is nothing within the LEB128 representation that indicates whether an
 * encoded number is signed or unsigned. The decoder must know what type of
 * number to expect.
 *
 * NOTE: It's assumed that LEB128 will not contain encodings for integers,
 * larger than 64 bit.
*/
typedef struct ELFF_PACKED Dwarf_Leb128 {
  /* Beginning of the LEB128 block. */
  Elf_Byte  val;

  /* Pulls actual value, encoded with this LEB128 block.
   * Param:
   *  value - Upon return will contain value, encoded with this LEB128 block.
   *  sign - If true, the caller expects the LEB128 to contain a signed
   *    integer, otherwise, caller expects an unsigned integer value to be
   *    encoded with this LEB128 block.
   */
  void get_common(Dwarf_Value* value, bool sign) const {
    value->u64 = 0;
    /* Integer zero is a special case. */
    if (val == 0) {
      value->type = sign ? DWARF_VALUE_S32 : DWARF_VALUE_U32;
      value->encoded_size = 1;
      return;
    }

    /* We've got to reconstruct the integer. */
    value->type = DWARF_VALUE_UNKNOWN;
    value->encoded_size = 0;

    /* Byte by byte loop though the LEB128, reconstructing the integer from
     * 7-bits chunks. Byte with 8-th bit set to zero indicates the end
     * of the LEB128 block. For signed integers, 7-th bit of the last LEB128
     * byte controls the sign. If 7-th bit of the last LEB128 byte is set,
     * the integer is negative. If 7-th bit of the last LEB128 byte is not
     * set, the integer is positive.
     */
    const Elf_Byte* cur = &val;
    Elf_Word shift = 0;
    while ((*cur & 0x80) != 0) {
      value->u64 |= (static_cast<Elf_Xword>(*cur) & 0x7F) << shift;
      shift += 7;
      value->encoded_size++;
      cur++;
    }
    value->u64 |= (static_cast<Elf_Xword>(*cur) & 0x7F) << shift;
    value->encoded_size++;

    /* LEB128 format doesn't carry any info of the sizeof of the integer it
     * represents. We well guess it, judging by the highest bit set in the
     * reconstucted integer.
     */
    if ((value->u64 & 0xFFFFFFFF00000000LL) == 0) {
      /* 32-bit integer. */
      if (sign) {
        value->type = DWARF_VALUE_S32;
        if (((*cur) & 0x40) != 0) {
          // Value is negative.
          value->u64 |= - (1 << (shift + 7));
        } else if ((value->u32 & 0x80000000) != 0) {
          // Make sure we don't report negative value in this case.
          value->type = DWARF_VALUE_S64;
        }
      } else {
        value->type = DWARF_VALUE_U32;
      }
    } else {
      /* 64-bit integer. */
      if (sign) {
        value->type = DWARF_VALUE_S64;
        if (((*cur) & 0x40) != 0) {
          // Value is negative.
          value->u64 |= - (1 << (shift + 7));
        }
      } else {
        value->type = DWARF_VALUE_U64;
      }
    }
  }

  /* Pulls actual unsigned value, encoded with this LEB128 block.
   * See get_common() for more info.
   * Param:
   *  value - Upon return will contain unsigned value, encoded with
   *  this LEB128 block.
   */
  void get_unsigned(Dwarf_Value* value) const {
    get_common(value, false);
  }

  /* Pulls actual signed value, encoded with this LEB128 block.
   * See get_common() for more info.
   * Param:
   *  value - Upon return will contain signed value, encoded with
   *  this LEB128 block.
   */
  void get_signed(Dwarf_Value* value) const {
    get_common(value, true);
  }

  /* Pulls LEB128 value, advancing past this LEB128 block.
   * See get_common() for more info.
   * Return:
   *  Pointer to the byte past this LEB128 block.
   */
  const void* process(Dwarf_Value* value, bool sign) const {
    get_common(value, sign);
    return INC_CPTR(&val, value->encoded_size);
  }

  /* Pulls LEB128 unsigned value, advancing past this LEB128 block.
   * See process() for more info.
   */
  const void* process_unsigned(Dwarf_Value* value) const {
    return process(value, false);
  }

  /* Pulls LEB128 signed value, advancing past this LEB128 block.
   * See process() for more info.
   */
  const void* process_signed(Dwarf_Value* value) const {
    return process(value, true);
  }
} Dwarf_Leb128;

/* DIE attribute descriptor in the .debug_abbrev section.
 * Attribute descriptor contains two LEB128 values. First one provides
 * attribute ID (one of DW_AT_XXX values), and the second one provides
 * format (one of DW_FORMAT_XXX values), in which attribute value is
 * encoded in the .debug_info section of the ELF file.
 */
typedef struct ELFF_PACKED Dwarf_Abbr_AT {
  /* Attribute ID (DW_AT_XXX).
   * Attribute format (DW_FORMAT_XXX) follows immediately.
   */
  Dwarf_Leb128  at;

  /* Checks if this is a separator descriptor.
   * Zero is an invalid attribute ID, indicating the end of attribute
   * list for the current DIE.
   */
  bool is_separator() const {
    return at.val == 0;
  }

  /* Pulls attribute data, advancing past this descriptor.
   * Param:
   *  at_value - Upon return contains attribute value of this descriptor.
   *  form - Upon return contains form value of this descriptor.
   * Return:
   *  Pointer to the byte past this descriptor block (usually, next
   *  attribute decriptor).
   */
  const Dwarf_Abbr_AT* process(Dwarf_At* at_value, Dwarf_Form* form) const {
    if (is_separator()) {
      /* Size of separator descriptor is always 2 bytes. */
      *at_value = 0;
      *form = 0;
      return INC_CPTR_T(Dwarf_Abbr_AT, &at.val, 2);
    }

    Dwarf_Value val;

    /* Process attribute ID. */
    const Dwarf_Leb128* next =
        reinterpret_cast<const Dwarf_Leb128*>(at.process_unsigned(&val));
    *at_value = val.u16;

    /* Follow with processing the form. */
    next = reinterpret_cast<const Dwarf_Leb128*>(next->process_unsigned(&val));
    *form = val.u16;
    return reinterpret_cast<const Dwarf_Abbr_AT*>(next);
  }
} Dwarf_Abbr_AT;

/* DIE abbreviation descriptor in the .debug_abbrev section.
 * DIE abbreviation descriptor contains three parameters. The first one is a
 * LEB128 value, that encodes 1 - based abbreviation descriptor number.
 * Abbreviation descriptor numbers seems to be always in sequential order, and
 * are counted on per-compilation unit basis. I.e. abbreviation number for the
 * first DIE abbreviation descriptor of each compilation unit is always 1.
 *
 * Besides abbreviation number, DIE abbreviation descriptor contains two more
 * values. The first one (after abbr_num) is a LEB128 value containing DIE's
 * tag value, and the second one is one byte flag specifying whether or not
 * the DIE contains any cildren.
 *
 * This descriptor is immediately followed by a list of attribute descriptors
 * (see Dwarf_Abbr_AT) for the DIE represented by this abbreviation descriptor.
 */
typedef struct ELFF_PACKED Dwarf_Abbr_DIE {
  /* 1 - based abbreviation number for the DIE. */
  Dwarf_Leb128  abbr_num;

  /* Gets abbreviation number for this descriptor. */
  Dwarf_AbbrNum get_abbr_num() const {
    Dwarf_Value val;
    abbr_num.get_unsigned(&val);
    return val.u16;
  }

  /* Gets DIE tag for this descriptor. */
  Dwarf_Tag get_tag() const {
    Dwarf_Tag tag;
    process(NULL, &tag);
    return tag;
  }

  /* Pulls DIE abbreviation descriptor data, advancing past this descriptor.
   * Param:
   *  abbr_index - Upon return contains abbreviation number for this
   *    descriptor. This parameter can be NULL, if the caller is not interested
   *    in this value.
   *  tag - Upon return contains tag of the DIE for this descriptor. This
   *    parameter can be NULL, if the caller is not interested in this value.
   *  form - Upon return contains form of the DIE for this descriptor.
   * Return:
   *  Pointer to the list of attribute descriptors for the DIE.
   */
  const Dwarf_Abbr_AT* process(Dwarf_AbbrNum* abbr_index,
                               Dwarf_Tag* tag) const {
    Dwarf_Value val;
    const Dwarf_Leb128* next =
        reinterpret_cast<const Dwarf_Leb128*>(abbr_num.process_unsigned(&val));
    if (abbr_index != NULL) {
      *abbr_index = val.u32;
    }

    /* Next one is a "tag". */
    next = reinterpret_cast<const Dwarf_Leb128*>(next->process_unsigned(&val));
    if (tag != NULL) {
      *tag = val.u16;
    }

    /* Next one is a "has children" one byte flag. We're not interested in it,
     * so jump to the list of attribute descriptors that immediately follows
     * this DIE descriptor. */
    return INC_CPTR_T(Dwarf_Abbr_AT, next, 1);
  }
} Dwarf_Abbr_DIE;

/* DIE descriptor in the .debug_info section.
 * DIE descriptor contains one LEB128-encoded value, containing DIE's
 * abbreviation descriptor number in the .debug_abbrev section.
 *
 * DIE descriptor is immediately followed by the list of DIE attribute values,
 * format of wich is defined by the list of attribute descriptors in the
 * .debug_abbrev section, that immediately follow the DIE attribute descriptor,
 * addressed by this descriptor's abbr_num LEB128.
 */
typedef struct ELFF_PACKED Dwarf_DIE {
  /* 1 - based index of DIE abbreviation descriptor (Dwarf_Abbr_DIE) for this
   * DIE in the .debug_abbrev section.
   *
   * NOTE: DIE abbreviation descriptor indexes are tied to the compilation
   * unit. In other words, each compilation unit restarts counting DIE
   * abbreviation descriptors from 1.
   *
   * NOTE: Zero is invalid value for this field, indicating that this DIE is a
   * separator (usually it ends a list of "child" DIEs)
   */
  Dwarf_Leb128  abbr_num;

  /* Checks if this is a separator DIE. */
  bool is_separator() const {
    return abbr_num.val == 0;
  }

  /* Gets (1 - based) abbreviation number for this DIE. */
  Dwarf_AbbrNum get_abbr_num() const {
    Dwarf_Value val;
    abbr_num.get_unsigned(&val);
    return val.u16;
  }

  /* Pulls DIE information, advancing past this descriptor to DIE attributes.
   * Param:
   *  abbr_num - Upon return contains abbreviation number for this DIE. This
   *    parameter can be NULL, if the caller is not interested in this value.
   * Return:
   *  Pointer to the byte past this descriptor (the list of DIE attributes).
   */
  const Elf_Byte* process(Dwarf_AbbrNum* abbr_number) const {
    if (is_separator()) {
      if (abbr_number != NULL) {
        *abbr_number = 0;
      }
      // Size of a separator DIE is 1 byte.
      return INC_CPTR_T(Elf_Byte, &abbr_num.val, 1);
    }
    Dwarf_Value val;
    const void* ret = abbr_num.process_unsigned(&val);
    if (abbr_number != NULL) {
      *abbr_number = val.u32;
    }
    return reinterpret_cast<const Elf_Byte*>(ret);
  }
} Dwarf_DIE;

/*
 * Variable size headers.
 * When encoding size value in DWARF, the first 32 bits of a "size" header
 * define header type. If first 32 bits of the header contain 0xFFFFFFFF
 * value, this is 64-bit size header with the following 64 bits encoding
 * the size. Otherwise, if first 32 bits are not 0xFFFFFFFF, they contain
 * 32-bit size value.
 */

/* Size header for 32-bit DWARF. */
typedef struct ELFF_PACKED Dwarf32_SizeHdr {
  /* Size value. */
  Elf_Word  size;
} Dwarf32_SizeHdr;

/* Size header for 64-bit DWARF. */
typedef struct ELFF_PACKED Dwarf64_SizeHdr {
  /* Size selector. For 64-bit DWARF this field is set to 0xFFFFFFFF */
  Elf_Word  size_selector;

  /* Actual size value. */
  Elf_Xword   size;
} Dwarf64_SizeHdr;

/* Compilation unit header in the .debug_info section.
 * Template param:
 *  Dwarf_SizeHdr - Type for the header's size field. Must be Dwarf32_SizeHdr
 *    for 32-bit DWARF, or Dwarf64_SizeHdr for 64-bit DWARF.
 *  Elf_Off - Type for abbrev_offset field. Must be Elf_Word for for 32-bit
 *    DWARF, or Elf_Xword for 64-bit DWARF.
 */
template <typename Dwarf_SizeHdr, typename Elf_Off>
struct ELFF_PACKED Dwarf_CUHdr {
  /* Size of the compilation unit data in .debug_info section. */
  Dwarf_SizeHdr   size_hdr;

  /* Compilation unit's DWARF version stamp. */
  Elf_Half        version;

  /* Relative (to the beginning of .debug_abbrev section data) offset of the
   * beginning of abbreviation sequence for this compilation unit.
   */
  Elf_Off         abbrev_offset;

  /* Pointer size for this compilation unit (should be 4, or 8). */
  Elf_Byte        address_size;
};
/* Compilation unit header in the .debug_info section for 32-bit DWARF. */
typedef Dwarf_CUHdr<Dwarf32_SizeHdr, Elf_Word> Dwarf32_CUHdr;
/* Compilation unit header in the .debug_info section for 64-bit DWARF. */
typedef Dwarf_CUHdr<Dwarf64_SizeHdr, Elf_Xword> Dwarf64_CUHdr;

/* CU STMTL header in the .debug_line section.
 * Template param:
 *  Dwarf_SizeHdr - Type for the header's size field. Must be Dwarf32_SizeHdr
 *    for 32-bit DWARF, or Dwarf64_SizeHdr for 64-bit DWARF.
 *  Elf_Size - Type for header_length field. Must be Elf_Word for for 32-bit
 *    DWARF, or Elf_Xword for 64-bit DWARF.
 */
template <typename Dwarf_SizeHdr, typename Elf_Size>
struct ELFF_PACKED Dwarf_STMTLHdr {
  /* The size in bytes of the line number information for this compilation
   * unit, not including the unit_length field itself. */
  Dwarf_SizeHdr unit_length;

  /* A version number. This number is specific to the line number information
   * and is independent of the DWARF version number. */
  Elf_Half      version;

  /* The number of bytes following the header_length field to the beginning of
   * the first byte of the line number program itself. In the 32-bit DWARF
   * format, this is a 4-byte unsigned length; in the 64-bit DWARF format,
   * this field is an 8-byte unsigned length. */
  Elf_Size      header_length;

  /* The size in bytes of the smallest target machine instruction. Line number
   * program opcodes that alter the address register first multiply their
   * operands by this value. */
  Elf_Byte      min_instruction_len;

  /* The initial value of the is_stmt register. */
  Elf_Byte      default_is_stmt;

  /* This parameter affects the meaning of the special opcodes. */
  Elf_Sbyte     line_base;

  /* This parameter affects the meaning of the special opcodes. */
  Elf_Byte      line_range;

  /* The number assigned to the first special opcode. */
  Elf_Byte      opcode_base;

  /* This is first opcode in an array specifying the number of LEB128 operands
   * for each of the standard opcodes. The first element of the array
   * corresponds to the opcode whose value is 1, and the last element
   * corresponds to the opcode whose value is opcode_base - 1. By increasing
   * opcode_base, and adding elements to this array, new standard opcodes can
   * be added, while allowing consumers who do not know about these new opcodes
   * to be able to skip them. NOTE: this array points to the mapped
   * .debug_line section. */
  Elf_Byte      standard_opcode_lengths;
};
/* CU STMTL header in the .debug_line section for 32-bit DWARF. */
typedef Dwarf_STMTLHdr<Dwarf32_SizeHdr, Elf_Word> Dwarf32_STMTLHdr;
/* CU STMTL header in the .debug_line section for 64-bit DWARF. */
typedef Dwarf_STMTLHdr<Dwarf64_SizeHdr, Elf_Xword> Dwarf64_STMTLHdr;

/* Source file descriptor in the .debug_line section.
 * Descriptor begins with zero-terminated file name, followed by an ULEB128,
 * encoding directory index in the list of included directories, followed by
 * an ULEB12, encoding file modification time, followed by an ULEB12, encoding
 * file size.
 */
typedef struct ELFF_PACKED Dwarf_STMTL_FileDesc {
  /* Zero-terminated file name. */
  char  file_name[1];

  /* Checks of this descriptor ends the list. */
  bool is_last_entry() const {
    return file_name[0] == '\0';
  }

  /* Gets file name. */
  const char* get_file_name() const {
    return file_name;
  }

  /* Processes this descriptor, advancing to the next one.
   * Param:
   *  dir_index - Upon return contains index of the parent directory in the
   *    list of included directories. Can be NULL if caller is not interested
   *    in this value.
   * Return:
   *  Pointer to the next source file descriptor in the list.
   */
  const Dwarf_STMTL_FileDesc* process(Elf_Word* dir_index) const {
    if (is_last_entry()) {
      return this;
    }

    /* First parameter: include directory index. */
    Dwarf_Value tmp;
    const Dwarf_Leb128* leb =
        INC_CPTR_T(Dwarf_Leb128, file_name, strlen(file_name) + 1);
    leb = reinterpret_cast<const Dwarf_Leb128*>(leb->process_unsigned(&tmp));
    if (dir_index != NULL) {
      *dir_index = tmp.u32;
    }
    /* Process file time. */
    leb = reinterpret_cast<const Dwarf_Leb128*>(leb->process_unsigned(&tmp));
    /* Process file size. */
    return reinterpret_cast<const Dwarf_STMTL_FileDesc*>(leb->process_unsigned(&tmp));
  }

  /* Gets directory index for this descriptor. */
  Elf_Word get_dir_index() const {
    assert(!is_last_entry());
    if (is_last_entry()) {
      return 0;
    }
    /* Get directory index. */
    Dwarf_Value ret;
    const Dwarf_Leb128* leb =
      INC_CPTR_T(Dwarf_Leb128, file_name, strlen(file_name) + 1);
    leb->process_unsigned(&ret);
    return ret.u32;
  }
} Dwarf_STMTL_FileDesc;

/* Encapsulates a DIE attribute, collected during ELF file parsing.
 */
class DIEAttrib {
 public:
  /* Constructs DIEAttrib intance. */
  DIEAttrib()
      : at_(0),
        form_(0) {
    value_.type = DWARF_VALUE_UNKNOWN;
  }

  /* Destructs DIEAttrib intance. */
  ~DIEAttrib() {
  }

  /* Gets DWARF attribute ID (DW_AT_Xxx) for this property. */
  Dwarf_At at() const {
    return at_;
  }

  /* Gets DWARF form ID (DW_FORM_Xxx) for this property. */
  Dwarf_Form form() const {
    return form_;
  }

  /* Gets value of this property. */
  const Dwarf_Value* value() const {
    return &value_;
  }

  /* Value of this property. */
  Dwarf_Value   value_;

  /* DWARF attribute ID (DW_AT_Xxx) for this property. */
  Dwarf_At      at_;

  /* DWARF form ID (DW_FORM_Xxx) for this property. */
  Dwarf_Form    form_;
};

/* Parse tag context.
 * This structure is used as an ELF file parsing parameter, limiting collected
 * DIEs by the list of tags.
 */
typedef struct DwarfParseContext {
  /* Zero-terminated list of tags to collect DIEs for. If this field is NULL,
   * DIEs for all tags will be collected during the parsing. */
  const Dwarf_Tag*  tags;
} DwarfParseContext;

/* Checks if a DIE with the given tag should be collected during the parsing.
 * Param:
 *  parse_context - Parse context to check the tag against. This parameter can
 *  be NULL, indicating that all tags should be collected.
 *  tag - Tag to check.
 * Return:
 *  true if a DIE with the given tag should be collected during the parsing,
 *  or false, if the DIE should not be collected.
 */
static inline bool
collect_die(const DwarfParseContext* parse_context, Dwarf_Tag tag) {
  if (parse_context == NULL || parse_context->tags == NULL) {
    return true;
  }
  for (const Dwarf_Tag* tags = parse_context->tags; *tags != 0; tags++) {
    if (*tags == tag) {
      return true;
    }
  }
  return false;
}

/* Encapsulates an array of Dwarf_Abbr_DIE pointers, cached for a compilation
 * unit. Although Dwarf_Abbr_DIE descriptors in the .debug_abbrev section of
 * the ELF file seems to be always in sequential order, DIE descriptors may
 * reference them randomly. So, to provide better performance, we will cache
 * all Dwarf_Abbr_DIE pointers, that were found for each DIE. Since all of the
 * Dwarf_Abbr_DIE are sequential, an array is the best way to cache them.
 *
 * NOTE: Objects of this class are instantiated one per each CU, as all DIE
 * abbreviation numberation is restarted from 1 for each new CU.
 */
class DwarfAbbrDieArray {
 public:
  /* Constructs DwarfAbbrDieArray instance.
   * Most of the CUs don't have too many unique Dwarf_Abbr_DIEs, so, in order
   * to decrease the amount of memory allocation calls, we will preallocate
   * a relatively small array for them along with the instance of this class,
   * hopping, that all Dwarf_Abbr_DIEs for the CU will fit into it.
   */
  DwarfAbbrDieArray()
      : array_(&small_array_[0]),
        array_size_(ELFF_ARRAY_SIZE(small_array_)),
        count_(0) {
  }

  /* Destructs DwarfAbbrDieArray instance. */
  ~DwarfAbbrDieArray() {
    if (array_ != &small_array_[0]) {
      delete[] array_;
    }
  }

  /* Adds new entry to the array
   * Param:
   *  abbr - New entry to add.
   *  num - Abbreviation number for the adding entry.
   *    NOTE: before adding, this method will verify that descriptor for the
   *    given abbreviation number has not been cached yet.
   *    NOTE: due to the nature of this array, entries MUST be added strictly
   *    in sequential order.
   * Return:
   *  true on success, false on failure.
   */
  bool add(const Dwarf_Abbr_DIE* abbr, Dwarf_AbbrNum num) {
    assert(num != 0);
    if (num == 0) {
      // Zero is illegal DIE abbreviation number.
      _set_errno(EINVAL);
      return false;
    }

    if (num <= count_) {
      // Already cached.
      return true;
    }

    // Enforce strict sequential order.
    assert(num == (count_ + 1));
    if (num != (count_ + 1)) {
      _set_errno(EINVAL);
      return false;
    }

    if (num >= array_size_) {
      /* Expand the array. Make it 64 entries bigger than adding entry number.
       * NOTE: that we don't check for an overflow here, since we secured
       * ourselves from that by enforcing strict sequential order. So, an
       * overflow may happen iff number of entries cached in this array is
       * close to 4G, which is a) totally unreasonable, and b) we would die
       * long before this amount of entries is cached.
       */
      Dwarf_AbbrNum new_size = num + 64;

      // Reallocate.
      const Dwarf_Abbr_DIE** new_array = new const Dwarf_Abbr_DIE*[new_size];
      assert(new_array != NULL);
      if (new_array == NULL) {
        _set_errno(ENOMEM);
        return false;
      }
      memcpy(new_array, array_, count_ * sizeof(const Dwarf_Abbr_DIE*));
      if (array_ != &small_array_[0]) {
        delete[] array_;
      }
      array_ = new_array;
      array_size_ = new_size;
    }

    // Abbreviation numbers are 1-based.
    array_[num - 1] = abbr;
    count_++;
    return true;
  }

  /* Adds new entry to the array
   * Param:
   *  abbr - New entry to add.
   * Return:
   *  true on success, false on failure.
   */
  bool add(const Dwarf_Abbr_DIE* abbr) {
    return add(abbr, abbr->get_abbr_num());
  }

  /* Gets an entry from the array
   * Param:
   *  num - 1-based index of an entry to get.
   * Return:
   *  Entry on success, or NULL if num exceeds the number of entries
   *  contained in the array.
   */
  const Dwarf_Abbr_DIE* get(Dwarf_AbbrNum num) const {
    assert(num != 0 && num <= count_);
    if (num != 0 && num <= count_) {
      return array_[num - 1];
    } else {
      _set_errno(EINVAL);
      return NULL;
    }
  }

  /* Caches Dwarf_Abbr_DIEs into this array up to the requested number.
   * NOTE: This method cannot be called on an empty array. Usually, first
   * entry is inserted into this array when CU object is initialized.
   * Param:
   *  num - Entry number to cache entries up to.
   * Return:
   *  Last cached entry (actually, an entry for the 'num' index).
   */
  const Dwarf_Abbr_DIE* cache_to(Dwarf_AbbrNum num) {
    /* Last cached DIE abbreviation. We always should have cached at least one
     * abbreviation for the CU DIE itself, added via "add" method when CU
     * object was initialized. */
    const Dwarf_Abbr_DIE* cur_abbr = get(count_);
    assert(cur_abbr != NULL);
    if (cur_abbr == NULL) {
      return NULL;
    }

    /* Starting with the last cached DIE abbreviation, loop through the
     * remaining DIE abbreviations in the .debug_abbrev section of the
     * mapped ELF file, caching them until we reach the requested
     * abbreviation descriptor number. Normally, the very next DIE
     * abbreviation will stop the loop. */
    while (num > count_) {
      Dwarf_AbbrNum abbr_num;
      Dwarf_Tag tmp2;
      Dwarf_Form tmp3;
      Dwarf_At tmp4;

      /* Process all AT abbreviations for the current DIE entry, reaching next
       * DIE abbreviation. */
      const Dwarf_Abbr_AT* abbr_at = cur_abbr->process(&abbr_num, &tmp2);
      while (!abbr_at->is_separator()) {
        abbr_at = abbr_at->process(&tmp4, &tmp3);
      }

      // Next DIE abbreviation is right after the separator AT abbreviation.
      cur_abbr = reinterpret_cast<const Dwarf_Abbr_DIE*>
                                              (abbr_at->process(&tmp4, &tmp3));
      if (!add(cur_abbr)) {
        return NULL;
      }
    }

    return array_[num - 1];
  }

  /* Empties array and frees allocations. */
  void empty() {
    if (array_ != &small_array_[0]) {
      delete[] array_;
      array_ = &small_array_[0];
      array_size_ = sizeof(small_array_) / sizeof(small_array_[0]);
    }
    count_ = 0;
  }

 protected:
  /* Array, preallocated in anticipation of relatively small number of
   * DIE abbreviations in compilation unit. */
  const Dwarf_Abbr_DIE*   small_array_[64];

  /* Array of Dwarf_Abbr_DIE pointers, cached for a compilation unit. */
  const Dwarf_Abbr_DIE**  array_;

  /* Current size of the array. */
  Dwarf_AbbrNum           array_size_;

  /* Number of entries, cached in the array. */
  Dwarf_AbbrNum           count_;
};

/* Encapsulates a state machine for the "Line Number Program", that is run
 * on data conained in the mapped .debug_line section.
 */
class DwarfStateMachine {
 public:
  /* Constructs DwarfStateMachine instance.
   * Param:
   *  set_is_stmt - Matches value of default_is_stmt field in the STMTL header.
   *    see Dwarf_STMTL_HdrXX.
   */
  explicit DwarfStateMachine(bool set_is_stmt)
    : address_(0),
      file_(1),
      line_(1),
      column_(0),
      discriminator_(0),
      is_stmt_(set_is_stmt),
      basic_block_(false),
      end_sequence_(false),
      prologue_end_(false),
      epilogue_begin_(false),
      isa_(0),
      set_file_info_(NULL) {
  }

  /* Destructs DwarfStateMachine instance. */
  ~DwarfStateMachine() {
  }

  /* Resets the state to default.
   * Param:
   *  set_is_stmt - Matches value of default_is_stmt field in the STMTL header.
   *    see Dwarf_STMTL_HdrXX.
  */
  void reset(bool set_is_stmt) {
    address_ = 0;
    file_ = 1;
    line_ = 1;
    column_ = 0;
    discriminator_ = 0;
    is_stmt_ = set_is_stmt;
    basic_block_ = false;
    end_sequence_ = false;
    prologue_end_ = false;
    epilogue_begin_ = false;
    isa_ = 0;
    set_file_info_ = NULL;
  }

  /*
   * Machine state.
   */

  /* Current address (current PC value). */
  Elf_Xword                   address_;

  /* Current index of source file descriptor. */
  Elf_Word                    file_;

  /* Current line in the current source file. */
  Elf_Word                    line_;

  /* Current column. */
  Elf_Word                    column_;

  /* Current discriminator value. */
  Elf_Word                    discriminator_;

  /* Current STMT flag. */
  bool                        is_stmt_;

  /* Current basic block flag. */
  bool                        basic_block_;

  /* Current end of sequence flag. */
  bool                        end_sequence_;

  /* Current end of prologue flag. */
  bool                        prologue_end_;

  /* Current epilogue begin flag. */
  bool                        epilogue_begin_;

  /* Current ISA value. */
  Elf_Word                    isa_;

  /* Current value for explicitly set current source file descriptor.
   * If not NULL, this descriptor has priority over the descriptor, addressed
   * by the file_ member of this class. */
  const Dwarf_STMTL_FileDesc* set_file_info_;
};

/* Checks if given tag belongs to a routine. */
static inline bool
dwarf_tag_is_routine(Dwarf_Tag tag) {
  return tag == DW_TAG_inlined_subroutine ||
         tag == DW_TAG_subprogram ||
         tag == DW_AT_main_subprogram;
}

/* Checks if given tag belongs to a compilation unit. */
static inline bool
dwarf_tag_is_cu(Dwarf_Tag tag) {
  return tag == DW_TAG_compile_unit ||
         tag == DW_TAG_partial_unit;
}

#endif  // ELFF_DWARF_DEFS_H_
