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
 * Contains declaration of a class DwarfCU, that encapsulates a compilation
 * unit in the .debug_info section of the mapped ELF file.
 */

#ifndef ELFF_DWARF_CU_H_
#define ELFF_DWARF_CU_H_

#include "dwarf_defs.h"
#include "dwarf_die.h"

/* Address information descriptor. */
typedef struct Dwarf_AddressInfo {
  /* Routine DIE containing the address. */
  const DIEObject*  die_obj;

  /* Source file name for the address. */
  const char*       file_name;

  /* Source file directory path for the address. */
  const char*       dir_name;

  /* Source file line number for the address. */
  Elf_Word          line_number;
} Dwarf_AddressInfo;

/* STMTL header cached by compilation unit. This header is contained in
 * the .debug_line section of the ELF file. */
typedef struct Dwarf_STMTL_Hdr {
  /* The size in bytes of the line number information for this compilation
   * unit, not including the unit_length field itself. */
  Elf_Xword                   unit_length;

  /* A version number. This number is specific to the line number information
   * and is independent of the DWARF version number. */
  Elf_Half                    version;

  /* The number of bytes following the header_length field to the beginning of
   * the first byte of the line number program itself. In the 32-bit DWARF
   * format, this is a 4-byte unsigned length; in the 64-bit DWARF format,
   * this field is an 8-byte unsigned length. */
  Elf_Xword                   header_length;

  /* The size in bytes of the smallest target machine instruction. Line number
   * program opcodes that alter the address register first multiply their
   * operands by this value. */
  Elf_Byte                    min_instruction_len;

  /* The initial value of the is_stmt register. */
  Elf_Byte                    default_is_stmt;

  /* This parameter affects the meaning of the special opcodes. */
  Elf_Sbyte                   line_base;

  /* This parameter affects the meaning of the special opcodes. */
  Elf_Byte                    line_range;

  /* The number assigned to the first special opcode. */
  Elf_Byte                    opcode_base;

  /* Points to standard_opcode_lengths array in the actual STMTL header in
   * the mapped .debug_line section. */
  const Elf_Byte*             standard_opcode_lengths;

  /* Pointer to the beginning of the list of include directories in the mapped
   * .debug_line section. */
  const char*                 include_directories;

  /* Number of include directories in the list that begins with
   * include_directories. */
  Elf_Word                    inc_dir_num;

  /* Pointer to the beginning of the list of file information in the mapped
   * .debug_line section. Each entry in this list begins with zero-terminated
   * file name, followed by ULEB128 encoding directory index for the file,
   * followed by ULEB128 encoding last modification time, followed by ULEB128
   * encoding length of file in bytes. */
  const Dwarf_STMTL_FileDesc* file_infos;

  /* Start of the "Line Number Program" in the mapped .debug_line section. */
  const Elf_Byte*             start;

  /* End of the "Line Number Program" in the mapped .debug_line section. */
  const Elf_Byte*             end;
} Dwarf_STMTL_Hdr;

/* Encapsulates architecture-independent functionality of a
 * compilation unit.
 */
class DwarfCU : public DwarfAllocBase {
friend class ElfFile;
 public:
  /* Constructs DwarfCU instance.
   * Param:
   *  elf - Instance of ElfFile containing this compilation unit.
   */
  explicit DwarfCU(ElfFile* elf);

  /* Destructs DwarfCU instance. */
  virtual ~DwarfCU();

  /* Creates DwarfCUImpl instance, depending on DWARF format.
   * Param:
   *  elf - Instance of ElfFile containing this compilation unit.
   *  hdr - Pointer to compilation unit header inside mapped .debug_info
   *    section of the ELF file. Actual data addressed by this pointer
   *    must be Dwarf32_CUHdr for 32 bit DWARFs, or Dwarf64_CUHdr for
   *    64 bit DWARFs.
   * Return:
   *  Created DwarfCUImpl instance (typecasted back to DwarfCU) on success,
   *  or NULL on failure.
   */
  static DwarfCU* create_instance(ElfFile* elf, const void* hdr);

  /* Process a DIE attribute.
   * Param:
   *  attr - Attribute list inside the mapped .debug_info section of the ELF
   *    file.
   *  form - Attribute's form, definig representation of attribute value in the
   *    mapped .debug_info section of the ELF file.
   *  attr_value - Upon return contains attribute value.
   * Return:
   *  Pointer to the next DIE attribute inside the mapped .debug_info section
   *  of the ELF file.
   */
  const Elf_Byte* process_attrib(const Elf_Byte* attr,
                                 Dwarf_Form form,
                                 Dwarf_Value* attr_value) const;

  /* Dumps this compilation unit to the stdout. */
  void dump() const;

  /* Gets instance of ElfFile containing this compilation unit. */
  ElfFile* elf_file() const {
    return elf_file_;
  }

  /* Gets DIE object for this CU. */
  DIEObject* cu_die() const {
    return cu_die_;
  }

  /* Gets byte size of the pointer type for this compilation unit. */
  Elf_Byte addr_sizeof() const {
    return addr_sizeof_;
  }

  /* Gets full path to the compilation directory (DW_AT_comp_dir attribute).
   * Return:
   *  Full path to the compilation directory (DW_AT_comp_dir attribute),
   *  or NULL, if that attribute was missing in CU's attribute list.
   */
  const char* comp_dir_path() const {
    DIEAttrib attr;
    return cu_die()->get_attrib(DW_AT_comp_dir, &attr) ? attr.value()->str :
                                                         NULL;
  }

  /* Gets relative (from the compilation directory) path to the compiled file.
   * (DW_AT_name attribute).
   * Return:
   *  Relative path to the compiled file (DW_AT_name attribute), or NULL, if
   *  that attribute was missing in CU's attribute list.
   */
  const char* rel_cu_path() const {
    DIEAttrib attr;
    return cu_die()->get_attrib(DW_AT_name, &attr) ? attr.value()->str :
                                                     NULL;
  }

  /* Gets next compilation unit in the list. NULL indicates the last CU. */
  DwarfCU* prev_cu() const {
    return prev_cu_;
  }

  /* Links this CU to the list of prevoiusly discovered CUs. */
  void set_prev_cu(DwarfCU* prev) {
    prev_cu_ = prev;
  }

  /* Checks if DWARF version for this CU is higher than 2. */
  bool is_DWARF3_or_higher() const {
    return version_ >= 3;
  }

  /* Gets DIE abbreviation for given abbreviation number.
   * See DwarfAbbrDieArray::get() */
  const Dwarf_Abbr_DIE* get_die_abbr(Dwarf_AbbrNum num) const {
    return abbrs_.get(num);
  }

  /* Gets DIE object containing given address.
   * DIE address ranges may overlap (for instance, address range for an inlined
   * routine will be contained within the address range of a routine where it
   * was inlined). This method will return a DIE object that is a "leaf" in
   * that inlining tree. I.e the returned DIE object represents the last DIE
   * in the branch of all DIEs containing given address.
   * Param:
   *  address - Address to get a DIE for. NOTE: for the sake of simplicity we
   *    explicitly use 64-bit type for an address.
   * Return:
   *  Leaf DIE containing given address, or NULL if this CU doesn't contain
   *  the given address.
   */
  DIEObject* get_leaf_die_for_address(Elf_Xword address) const {
    return cu_die_->get_leaf_for_address(address);
  }

  /* Checks if this CU contains 64, or 32-bit addresses. */
  bool is_CU_address_64() const {
    return addr_sizeof_ == 8;
  }
  bool is_CU_address_32() const {
    return addr_sizeof_ == 4;
  }

//=============================================================================
// DWARF format dependent methods
//=============================================================================

 public:
  /* Parses this compilation unit in .debug_info section, collecting children
   * DIEs of this compilation unit.
   * Param:
   *  parse_context - Parsing context that lists tags for DIEs that should be
   *    collected during parsing. NULL passed in this parameter indicates DIEs
   *    for all tags should be collected.
   *  next_cu_die - Upon successful return contains pointer to the next
   *    compilation unit descriptor inside mapped .debug_info section of
   *    the ELF file.
   * Return:
   *  true on success, false on failure.
   */
  virtual bool parse(const DwarfParseContext* parse_context,
                     const void** next_cu_die) = 0;

  /* Gets a DIE object referenced by an offset from the beginning of
   * this CU in the mapped .debug_info section.
   */
  virtual DIEObject* get_referenced_die_object(Elf_Word ref) const = 0;

  /* Gets a reference to a DIE object (offset of the DIE from the
   * beginning of this CU in the mapped .debug_info section.
   */
  virtual Elf_Word get_die_reference(const Dwarf_DIE* die) const = 0;

  /* Gets PC address information.
   * Param:
   *  address - PC address to get information for.
   *  info - Upon success contains information about routine that belongs to
   *    this compilation unit, containing the given address.
   * Return:
   *  true on success, or false if this CU doesn't contain the given address.
   */
  virtual bool get_pc_address_file_info(Elf_Xword address,
                                        Dwarf_AddressInfo* info) = 0;

  /* Gets file descriptor in the mapped .debug_line section of ELF file for a
   * given index in the file descriptor list.
   * Param:
   *  index - 1-based index of file descriptor in the file descriptor list.
   * Return:
   *  File descriptor for the given index, or NULL if index was too big.
   *  NOTE: pointer returned from this method addressed mapped section of
   *  ELF file.
   */
  virtual const Dwarf_STMTL_FileDesc* get_stmt_file_info(Elf_Word index) = 0;

  /* Gets directory name by an index in the mapped .debug_line section of
   * ELF file.
   * Param:
   *  dir_index - Index of the directory in the file descriptor list. If this
   *    parameter is zero, compilation directory (DW_AT_comp_dir) for this CU
   *    will be returned.
   * Return:
   *  Directory name for the given index, or NULL if index was too big.
   *  NOTE: pointer returned from this method addressed mapped section of
   *  ELF file.
   */
  virtual const char* get_stmt_dir_name(Elf_Word dir_index) = 0;

 protected:
  /* DIE abbreviation descriptors, cached for this compilation unit. */
  DwarfAbbrDieArray   abbrs_;

  /* Instance of an ELF file that contains this compilation unit. */
  ElfFile*            elf_file_;

  /* DIE object for this CU. */
  DIEObject*          cu_die_;

  /* Next compilation unit in the list (previous in the order they've been
   * discovered during ELF file parsing).
   */
  DwarfCU*            prev_cu_;

  /* DWARF version for this CU. */
  Elf_Half            version_;

  /* Byte size of the pointer type for this compilation unit. */
  Elf_Byte            addr_sizeof_;
};

/* Encapsulates architecture-dependent functionality of a compilation unit.
 * Template param:
 *  Dwarf_CUHdr - type compilation unit header in the mapped .debug_info
 *    section of ELF file. Must be:
 *    - Dwarf32_CUHdr for 32-bit DWARF, or
 *    - Dwarf64_CUHdr for 64-bit DWARF.
 *  Dwarf_Off - type for an offset field in DWARF data format. Must be:
 *    - Dwarf32_Off for 32-bit DWARF, or
 *    - Dwarf64_Off for 64-bit DWARF.
 */
template <typename Dwarf_CUHdr, typename Dwarf_Off>
class DwarfCUImpl : public DwarfCU {
 public:
  /* Constructs DwarfCU instance.
   * Param:
   *  elf - Instance of ElfFile containing this compilation unit.
   *  hdr - Pointer to compilation unit header inside mapped .debug_info
   *    section of the ELF file.
   */
  DwarfCUImpl(ElfFile* elf, const Dwarf_CUHdr* hdr);

  /* Destructs DwarfCU instance. */
  ~DwarfCUImpl() {
  }

  /* Parses this compilation unit in .debug_info section, collecting children
   * DIEs of this compilation unit. This is an implementation of DwarfCU's
   * abstract metod.
   * See DwarfCU::parse().
   */
  bool parse(const DwarfParseContext* parse_context,
             const void** next_cu_die);

  /* Gets PC address information.
   * This is an implementation of DwarfCU's abstract metod.
   * See DwarfCU::get_pc_address_file_info().
   */
  bool get_pc_address_file_info(Elf_Xword address, Dwarf_AddressInfo* info);

  /* Gets file descriptor in the mapped .debug_line section of ELF file for a
   * given index in the file descriptor list.
   * This is an implementation of DwarfCU's abstract metod.
   * See DwarfCU::get_stmt_file_info().
   */
  const Dwarf_STMTL_FileDesc* get_stmt_file_info(Elf_Word index);

  /* Gets directory name by an index in the mapped .debug_line section of
   * ELF file.
   * This is an implementation of DwarfCU's abstract metod.
   * See DwarfCU::get_stmt_dir_name().
   */
  const char* get_stmt_dir_name(Elf_Word dir_index);

  /* Gets a DIE object referenced by an offset from the beginning of
   * this CU. This is an implementation of DwarfCU's abstract metod.
   */
  DIEObject* get_referenced_die_object(Elf_Word ref) const {
    const Dwarf_DIE* die = get_referenced_die(ref);
    return cu_die_->find_die_object(die);
  }

  /* Gets a reference to a DIE object (offset of the DIE from the
   * beginning of this CU in the mapped .debug_info section.
   * This is an implementation of DwarfCU's abstract metod.
   */
  Elf_Word get_die_reference(const Dwarf_DIE* die) const {
    return static_cast<Elf_Word>(diff_ptr(cu_header_, die));
  }

 protected:
  /* Process a child DIE (and all its children) in this compilation unit.
   * Param:
   *  parse_context - See DwarfCU::parse().
   *  die - DIE descriptor of the child to process in this method.
   *  parent_obj - Parent object of the child to process in this method.
   *    NOTE: this parameter can be NULL only for a DIE that represents this
   *    compilation unit itself.
   * Return:
   *  Pointer to the end of child's attribute list in the mapped .debug_info
   *  section on success, or NULL on failure. Usually, pointer returned from
   *  this method is simply discarded, since parent calculates address of the
   *  next sibling's DIE based on DW_AT_sibling attribute of the DIE preceding
   *  child's DIE.
   */
  const Elf_Byte* process_DIE(const DwarfParseContext* parse_context,
                              const Dwarf_DIE* die,
                              DIEObject* parent_obj);

  /* Creates a DIE object for the given DIE.
   * Param:
   *  parse_context See DwarfCU::parse().
   *  die - DIE to create an object for.
   *  parent - Parent DIE object for the one that's being created in this
   *    method.
   *  tag - Tag of the DIE object that's being created in this method.
   * Return:
   *  Created DIE object. This method may returns NULL in two cases:
   *    - We're not interested in this DIE (decided by looking at 'tag'
   *      parameter. In this case errno should be set to zero.
   *    - Memory allocation has failed. In this case errno should be
   *      set to ENOMEM.
   */
  DIEObject* create_die_object(const DwarfParseContext* parse_context,
                               const Dwarf_DIE* die,
                               DIEObject* parent,
                               Dwarf_Tag tag);

  /* Initializes (caches) STMT lines header for this CU. */
  bool init_stmtl();

  /* Saves current source file information, collected in the state machine by
   * the "Line Number Program".
   * Param:
   *  state - State machine collected "Line Number Program" results.
   *  info - Upon success contains source file information, copied over from
   *    the state machine.
   * Return:
   *  true on success, or false on failure.
   */
  bool set_source_info(const DwarfStateMachine* state,
                       Dwarf_AddressInfo* info);

  /* Gets pointer to the DIE descriptor for this CU. */
  const Dwarf_DIE* get_DIE() const {
    /* CU's DIE descriptor immediately follows CU header. */
    return INC_CPTR_T(Dwarf_DIE, cu_header_, sizeof(Dwarf_CUHdr));
  }

  /* Caches STMTL header from .debug_line section to stmtl_header_.
   * Template param:
   *  Dwarf_STMTL_Hdr - Dwarf_STMTL_Hdr32, or Dwarf_STMTL_Hdr64, depending
   *    on the header type.
   * Param:
   *  stmtl_hdr - STMTL header in the mapped .debug_line section to cache.
   */
  template <typename Dwarf_STMTL_Hdr>
  void cache_stmtl(const Dwarf_STMTL_Hdr* stmtl_hdr) {
    stmtl_header_.unit_length = elf_file()->pull_val(stmtl_hdr->unit_length.size);
    stmtl_header_.version = elf_file()->pull_val(stmtl_hdr->version);
    stmtl_header_.header_length = elf_file()->pull_val(stmtl_hdr->header_length);
    stmtl_header_.min_instruction_len = stmtl_hdr->min_instruction_len;
    stmtl_header_.default_is_stmt = stmtl_hdr->default_is_stmt;
    stmtl_header_.line_base = stmtl_hdr->line_base;
    stmtl_header_.line_range = stmtl_hdr->line_range;
    stmtl_header_.opcode_base = stmtl_hdr->opcode_base;
    stmtl_header_.standard_opcode_lengths = &stmtl_hdr->standard_opcode_lengths;
    stmtl_header_.start = INC_CPTR_T(Elf_Byte, &stmtl_hdr->min_instruction_len,
                                     stmtl_header_.header_length);
    stmtl_header_.end = INC_CPTR_T(Elf_Byte, &stmtl_hdr->version,
                                   stmtl_header_.unit_length);
    stmtl_header_.include_directories =
        INC_CPTR_T(char, stmtl_header_.standard_opcode_lengths,
                   stmtl_header_.opcode_base - 1);
    const char* dir = stmtl_header_.include_directories;
    while (*dir != '\0') {
      dir += strlen(dir) + 1;
      stmtl_header_.inc_dir_num++;
    }
    stmtl_header_.file_infos = INC_CPTR_T(Dwarf_STMTL_FileDesc, dir, 1);
  }

  /* Gets a DIE referenced by an offset from the beginning of this CU
   * in the mapped .debug_info section.
   */
  const Dwarf_DIE* get_referenced_die(Elf_Word ref) const {
    return INC_CPTR_T(Dwarf_DIE, cu_header_, ref);
  }

  /* Checks if pointer to the DIE attribute is contained within the CU's area
   * of the mapped .debug_info section.
   * Param:
   *  ptr - Pointer to the DIE attribute to check.
   * Return:
   *  true, if pointer to the DIE attribute is contained within the CU's area
   *  of the mapped .debug_info section, or false if attribute pointer goes
   *  beyond CU's area of the mapped .debug_info section.
   */
  bool is_attrib_ptr_valid(const void* ptr) const {
    return diff_ptr(cu_header_, ptr) < cu_size_;
  }

 protected:
  /* Pointer to this compilation unit header inside the mapped .debug_info
   * section of the ELF file.
   */
  const Dwarf_CUHdr*          cu_header_;

  /* Size of this compilation unit area in the mapped .debug_info section.
   * This value has been cached off the CU header in order to avoid
   * endianness conversions.
   */
  Dwarf_Off                   cu_size_;

  /* STMT lines header, cached off mapped .debug_line section. */
  Dwarf_STMTL_Hdr             stmtl_header_;
};

#endif  // ELFF_DWARF_CU_H_
