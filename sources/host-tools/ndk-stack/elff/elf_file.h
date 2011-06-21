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
 * Contains declaration of ElfFile classes that encapsulate an ELF file.
 */

#ifndef ELFF_ELF_FILE_H_
#define ELFF_ELF_FILE_H_

#include "dwarf_die.h"
#include "elf_mapped_section.h"
#include "elff_api.h"
#include "mapfile.h"

/* Encapsulates architecture-independent functionality of an ELF file.
 *
 * This class is a base class for templated ElfFileImpl. This class implements
 * functionality around an ELF file that is independent from particulars of the
 * ELF's CPU architectire, while ElfFileImpl handles all particulars of CPU
 * architecture (namely, 32 or 64-bit), for which ELF file has been built.
 *
 * NOTE: This class operates on ELF sections that have been mapped to memory.
 *
 */
class ElfFile {
 public:
  /* Constructs ElfFile instance. */
  ElfFile();

  /* Destructs ElfFile instance. */
  virtual ~ElfFile();

  /* Creates ElfFileImpl instance, depending on ELF file CPU architecture.
   * This method will collect initial information about requested ELF file,
   * and will instantiate appropriate ElfFileImpl class object for it.
   * Param:
   *  path - Full path to the ELF file.
   * Return:
   *  Initialized ElfFileImpl instance, typecasted back to ElfFile object on
   *  success, or NULL on failure, with errno providing extended error
   *  information.
   */
  static ElfFile* Create(const char* path);

  /* Checks if ELF file is a 64, or 32-bit ELF file. */
  bool is_ELF_64() const {
    return is_ELF_64_;
  }
  bool is_ELF_32() const {
    return !is_ELF_64_;
  }

  /* Checks if ELF file data format is big, or little-endian. */
  bool is_elf_big_endian() const {
    return is_elf_big_endian_;
  }
  bool is_elf_little_endian() const {
    return !is_elf_big_endian_;
  }

  /* Checks whether or not endianness of CPU this library is built for matches
   * endianness of the ELF file that is represented with this instance. */
  bool same_endianness() const {
    return same_endianness_;
  }

  /* Checks if format of DWARF data in this file is 64, or 32-bit. */
  bool is_DWARF_64() const {
    return is_DWARF_64_;
  }
  bool is_DWARF_32() const {
    return !is_DWARF_64_;
  }

  /* Gets DWARF objects allocator for this instance. */
  class ElfAllocator* allocator() const {
    return allocator_;
  }

  /* Gets head of compilation unit list, collected during parsing of this file.
   * NOTE: list of collected compilation units returned from this method is
   * in reverse order relatively to the order CUs have been added to the list
   * during ELF file parsing.
   */
  class DwarfCU* last_cu() const {
    return last_cu_;
  }

  /* Gets number of compilation units, collected during parsing of
   * this ELF file with parse_compilation_units() method.
   */
  int cu_count() const {
    return cu_count_;
  }

  /* Gets  executable file flag */
  bool is_exec() const {
      return is_exec_;
  }

 protected:
  /* Initializes ElfFile instance. This method is called from Create method of
   * this class after appropriate ElfFileImpl instance has been created. Note,
   * that Create() method will validate that requested file is an ELF file,
   * prior to instantiating of an ElfFileImpl object, and calling this method.
   * Param:
   *  elf_hdr - Address of the common ELF file header.
   *  path - See Create().
   * Return:
   *  true on success, or false on failure, with errno containing extended
   *  error information.
   */
  virtual bool initialize(const Elf_CommonHdr* elf_hdr, const char* path);

/*=============================================================================
 * Endianness helper methods.
 * Since endianness of ELF file may differ from the endianness of the CPU this
 * library runs on, every time a value is required from a section of the ELF
 * file, it must be first pulled out of that section to a local variable, and
 * then used from that local variable. While value is pulled from ELF file
 * section, it must be converted accordingly to the endianness of the CPU and
 * ELF file. Routines bellow provide such functionality.
=============================================================================*/

 public:
  /* Pulls one byte value from ELF file. Note that for one byte we don't need
   * to do any endianness conversion, and these two methods are provided purely
   * for completness of the API.
   * Param:
   *  val - References value inside ELF file buffer to pull data from.
   * Return
   *  Pulled value with endianness appropriate for the CPU this library is
   *  running on.
   */
  uint8_t pull_val(const uint8_t* val) const {
    return *val;
  }
  uint8_t pull_val(const uint8_t& val) const {
    return val;
  }
  int8_t pull_val(const int8_t* val) const {
    return *val;
  }
  int8_t pull_val(const int8_t& val) const {
    return val;
  }

  /* Pulls two byte value from ELF file.
   * Param:
   *  val - References value inside ELF file buffer to pull data from.
   * Return
   *  Pulled value with endianness appropriate for the CPU this library is
   *  running on.
   */
  uint16_t pull_val(const uint16_t* val) const {
    if (same_endianness()) {
      return *val;
    }
    if (is_elf_big_endian()) {
      return (uint16_t)get_byte(val, 0) << 8 | get_byte(val, 1);
    } else {
      return (uint16_t)get_byte(val, 1) << 8 | get_byte(val, 0);
    }
  }
  uint16_t pull_val(const uint16_t& val) const {
    return same_endianness() ? val : pull_val(&val);
  }
  int16_t pull_val(const int16_t* val) const {
    return static_cast<int16_t>
              (pull_val(reinterpret_cast<const uint16_t*>(val)));
  }
  int16_t pull_val(const int16_t& val) const {
    return static_cast<int16_t>
              (pull_val(reinterpret_cast<const uint16_t&>(val)));
  }

  /* Pulls four byte value from ELF file.
   * Param:
   *  val - References value inside ELF file buffer to pull data from.
   * Return
   *  Pulled value with endianness appropriate for the CPU this library is
   *  running on.
   */
  uint32_t pull_val(const uint32_t* val) const {
    if (same_endianness()) {
      return *val;
    }
    if (is_elf_big_endian()) {
      return (uint32_t)get_byte(val, 0) << 24 |
             (uint32_t)get_byte(val, 1) << 16 |
             (uint32_t)get_byte(val, 2) << 8  |
             (uint32_t)get_byte(val, 3);
    } else {
      return (uint32_t)get_byte(val, 3) << 24 |
             (uint32_t)get_byte(val, 2) << 16 |
             (uint32_t)get_byte(val, 1) << 8  |
             (uint32_t)get_byte(val, 0);
    }
  }
  uint32_t pull_val(const uint32_t& val) const {
    return same_endianness() ? val : pull_val(&val);
  }
  int32_t pull_val(const int32_t* val) const {
    return static_cast<int32_t>
              (pull_val(reinterpret_cast<const uint32_t*>(val)));
  }
  int32_t pull_val(const int32_t& val) const {
    return static_cast<int32_t>
              (pull_val(reinterpret_cast<const uint32_t&>(val)));
  }

  /* Pulls eight byte value from ELF file.
   * Param:
   *  val - References value inside ELF file buffer to pull data from.
   * Return
   *  Pulled value with endianness appropriate for the CPU this library is
   *  running on.
   */
  uint64_t pull_val(const uint64_t* val) const {
    if (same_endianness()) {
      return *val;
    }
    if (is_elf_big_endian()) {
      return (uint64_t)get_byte(val, 0) << 56 |
             (uint64_t)get_byte(val, 1) << 48 |
             (uint64_t)get_byte(val, 2) << 40 |
             (uint64_t)get_byte(val, 3) << 32 |
             (uint64_t)get_byte(val, 4) << 24 |
             (uint64_t)get_byte(val, 5) << 16 |
             (uint64_t)get_byte(val, 6) << 8  |
             (uint64_t)get_byte(val, 7);
    } else {
      return (uint64_t)get_byte(val, 7) << 56 |
             (uint64_t)get_byte(val, 6) << 48 |
             (uint64_t)get_byte(val, 5) << 40 |
             (uint64_t)get_byte(val, 4) << 32 |
             (uint64_t)get_byte(val, 3) << 24 |
             (uint64_t)get_byte(val, 2) << 16 |
             (uint64_t)get_byte(val, 1) << 8  |
             (uint64_t)get_byte(val, 0);
    }
  }
  uint64_t pull_val(const uint64_t& val) const {
    return same_endianness() ? val : pull_val(&val);
  }
  int64_t pull_val(const int64_t* val) const {
    return static_cast<int64_t>
              (pull_val(reinterpret_cast<const uint64_t*>(val)));
  }
  int64_t pull_val(const int64_t& val) const {
    return static_cast<int64_t>
              (pull_val(reinterpret_cast<const uint64_t&>(val)));
  }

//=============================================================================
// ELF file section management.
//=============================================================================

 public:
  /* Gets a string contained in ELF's string section by index.
   * Param:
   *  index - String index (byte offset) in the ELF's string section.
   * Return:
   *  Pointer to the requested string, or NULL if string index exceeds ELF's
   *  string section size.
   *  NOTE: pointer returned from this method points to a mapped section of
   *  ELF file.
   */
  const char* get_str_sec_str(Elf_Xword index) const {
    assert(string_section_.is_mapped() && index < string_section_.size());
    if (string_section_.is_mapped() && index < string_section_.size()) {
      return INC_CPTR_T(char, string_section_.data(), index);
    } else {
      _set_errno(EINVAL);
      return NULL;
    }
  }

  /* Gets a string contained in ELF's debug string section (.debug_str)
   * by index.
   * Param:
   *  index - String index (byte offset) in the ELF's debug string section.
   * Return:
   *  Pointer to the requested string, or NULL if string index exceeds ELF's
   *  debug string section size.
   *  NOTE: pointer returned from this method points to a mapped section of
   *  ELF file.
   */
  const char* get_debug_str(Elf_Xword index) const {
    assert(debug_str_.is_mapped() && index < debug_str_.size());
    if (debug_str_.is_mapped() && index < debug_str_.size()) {
      return INC_CPTR_T(char, debug_str_.data(), index);
    } else {
      _set_errno(EINVAL);
      return NULL;
    }
  }

 protected:
  /* Gets pointer to a section header, given section index within ELF's
   * section table.
   * Param:
   *  index - Section index within ELF's section table.
   * Return:
   *  Pointer to a section header (ElfXX_SHdr flavor, depending on ELF's CPU
   *  architecture) on success, or NULL if section index exceeds number of
   *  sections for this ELF file.
   */
  const void* get_section_by_index(Elf_Half index) const {
    assert(index < sec_count_);
    if (index < sec_count_) {
      return INC_CPTR(sec_table_, static_cast<size_t>(index) * sec_entry_size_);
    } else {
      _set_errno(EINVAL);
      return NULL;
    }
  }

//=============================================================================
// DWARF management.
//=============================================================================

 protected:
  /* Parses DWARF, and buids a list of compilation units for this ELF file.
   * Compilation unit, collected with this methods are linked together in a
   * list, head of which is available via last_cu() method of this class.
   * NOTE: CUs in the list returned via last_cu() method are in reverse order
   * relatively to the order in which CUs are stored in .debug_info section.
   * This is ELF and DWARF data format - dependent method.
   * Param:
   *  parse_context - Parsing context that defines which tags, and which
   *    properties for which tag should be collected during parsing. NULL
   *    passed in this parameter indicates that all properties for all tags
   *    should be collected.
   * Return:
   *  Number of compilation units, collected in this method on success,
   *  or -1 on failure.
   */
  virtual int parse_compilation_units(const DwarfParseContext* parse_context) = 0;

 public:
  /* Gets PC address information.
   * Param:
   *  address - PC address to get information for. The address must be relative
   *    to the beginning of ELF file represented by this class.
   *  address_info - Upon success contains information about routine(s) that
   *    contain the given address.
   * Return:
   *  true if routine(s) containing has been found and its information has been
   *  saved into address_info, or false if no appropriate routine for that
   *  address has been found, or there was a memory error when collecting
   *  routine(s) information. In case of failure, errno contains extended error
   *  information.
   */
  bool get_pc_address_info(Elf_Xword address, Elf_AddressInfo* address_info);

  /* Frees resources aqcuired for address information in successful call to
   * get_pc_address_info().
   * Param:
   *  address_info - Address information structure, initialized in successful
   *    call to get_pc_address_info() routine.
   */
  void free_pc_address_info(Elf_AddressInfo* address_info) const;

  /* Gets beginning of the .debug_info section data.
   * Return:
   *  Beginning of the .debug_info section data.
   *  NOTE: pointer returned from this method points to a mapped section of
   *  ELF file.
   */
  const void* get_debug_info_data() const {
    return debug_info_.data();
  }

  /* Gets beginning of the .debug_abbrev section data.
   * Return:
   *  Beginning of the .debug_abbrev section data.
   *  NOTE: pointer returned from this method points to a mapped section of
   *  ELF file.
   */
  const void* get_debug_abbrev_data() const {
    return debug_abbrev_.data();
  }

  /* Gets beginning of the .debug_ranges section data.
   * Return:
   *  Beginning of the .debug_ranges section data.
   *  NOTE: pointer returned from this method points to a mapped section of
   *  ELF file.
   */
  const void* get_debug_ranges_data() const {
    return debug_ranges_.data();
  }

  /* Gets beginning of the .debug_line section data.
   * Return:
   *  Beginning of the .debug_line section data.
   *  NOTE: pointer returned from this method points to a mapped section of
   *  ELF file.
   */
  const void* get_debug_line_data() const {
    return debug_line_.data();
  }

  /* Checks, if given address range is contained in the mapped .debug_info
   * section of this file.
   * Param:
   *  ptr - Starting address of the range.
   *  size - Range size in bytes.
   * Return:
   *  true if given address range is contained in the mapped .debug_info
   *  section of this file, or false if any part of the range doesn't belong
   *  to that section.
   */
  bool is_valid_die_ptr(const void* ptr, size_t size) const {
    return debug_info_.is_contained(ptr, size);
  }

  /* Checks, if given address range is contained in the mapped .debug_abbrev
   * section of this file.
   * Param:
   *  ptr - Starting address of the range.
   *  size - Range size in bytes.
   * Return:
   *  true if given address range is contained in the mapped .debug_abbrev
   *  section of this file, or false if any part of the range doesn't belong
   *  to that section.
   */
  bool is_valid_abbr_ptr(const void* ptr, size_t size) const {
    return debug_abbrev_.is_contained(ptr, size);
  }

  /* Checks if given pointer addresses a valid compilation unit header in the
   * mapped .debug_info section of the ELF file.
   * Param:
   *  cu_header - Pointer to a compilation unit header to check.
   * Return
   *  true, if given pointer addresses a valid compilation unit header, or
   *  false, if it's not. A valid CU header must be fully conained inside
   *  .debug_info section of the ELF file, and its size must not be zero.
   */
  bool is_valid_cu(const void* cu_header) const {
    if (is_DWARF_64()) {
      return is_valid_die_ptr(cu_header, sizeof(Dwarf64_CUHdr)) &&
             reinterpret_cast<const Dwarf64_CUHdr*>(cu_header)->size_hdr.size != 0;
    } else {
      return is_valid_die_ptr(cu_header, sizeof(Dwarf32_CUHdr)) &&
             reinterpret_cast<const Dwarf32_CUHdr*>(cu_header)->size_hdr.size != 0;
    }
  }

  /* Gets range's low and high pc for the given range reference in the mapped
   * .debug_ranges section of an ELF file.
   * Template param:
   *  AddrType - Defines pointer type for the CU the range belongs to. CU's
   *    pointer type can be defined independently from ELF and DWARF types,
   *    and is encoded in address_size field of the CU header in .debug_info
   *    section of ELF file.
   * Param:
   *  offset - Byte offset within .debug_ranges section of the range record.
   *  low - Upon successful return contains value for range's low pc.
   *  high - Upon successful return contains value for range's high pc.
   * Return:
   *  true on success, or false, if requested record is not fully contained
   *  in the .debug_ranges section.
   */
  template<typename AddrType>
  bool get_range(Elf_Word offset, AddrType* low, AddrType* high) {
    const AddrType* ptr = INC_CPTR_T(AddrType, debug_ranges_.data(), offset);
    assert(debug_ranges_.is_contained(ptr, sizeof(AddrType) * 2));
    if (!debug_ranges_.is_contained(ptr, sizeof(AddrType) * 2)) {
      _set_errno(EINVAL);
      return false;
    }
    *low = pull_val(ptr);
    *high = pull_val(ptr + 1);
    return true;
  }

 protected:
  /* Mapped ELF string section. */
  ElfMappedSection    string_section_;

  /* Mapped .debug_info section. */
  ElfMappedSection    debug_info_;

  /* Mapped .debug_abbrev section. */
  ElfMappedSection    debug_abbrev_;

  /* Mapped .debug_str section. */
  ElfMappedSection    debug_str_;

  /* Mapped .debug_line section. */
  ElfMappedSection    debug_line_;

  /* Mapped .debug_ranges section. */
  ElfMappedSection    debug_ranges_;

  /* Base address of the loaded module (if fixed), or 0 if module doesn't get
   * loaded at fixed address. */
  Elf_Xword           fixed_base_address_;

  /* Handle to the ELF file represented with this instance. */
  MapFile*            elf_handle_;

  /* Path to the ELF file represented with this instance. */
  char*               elf_file_path_;

  /* DWARF objects allocator for this instance. */
  class ElfAllocator* allocator_;

  /* Beginning of the cached ELF's section table. */
  void*               sec_table_;

  /* Number of sections in the ELF file wrapped by this instance. */
  Elf_Half            sec_count_;

  /* Byte size of an entry in the section table. */
  Elf_Half            sec_entry_size_;

  /* Head of compilation unit list, collected during the parsing. */
  class DwarfCU*      last_cu_;

  /* Number of compilation units in last_cu_ list. */
  int                 cu_count_;

  /* Flags ELF's CPU architecture: 64 (true), or 32 bits (false). */
  bool                is_ELF_64_;

  /* Flags endianness of the processed ELF file. true indicates that ELF file
   * data is stored in big-endian form, false indicates that ELF file data is
   * stored in big-endian form.
   */
  bool                is_elf_big_endian_;

  /* Flags whether or not endianness of CPU this library is built for matches
   * endianness of the ELF file that is represented with this instance.
   */
  bool                same_endianness_;

  /* Flags DWARF format: 64, or 32 bits. DWARF format is determined by looking
   * at the first 4 bytes of .debug_info section (which is the beginning of the
   * first compilation unit header). If first 4 bytes contain 0xFFFFFFFF, the
   * DWARF is 64 bit. Otherwise, DWARF is 32 bit. */
  bool                is_DWARF_64_;

  /* Flags executable file. If this member is 1, ELF file represented with this
   * instance is an executable. If this member is 0, file is a shared library.
   */
  bool                is_exec_;
};

/* Encapsulates architecture-dependent functionality of an ELF file.
 * Template param:
 *  Elf_Addr - type for an address field in ELF file. Must be:
 *    - Elf32_Addr for 32-bit CPU, or
 *    - Elf64_Addr for 64-bit CPU.
 *  Elf_Off - type for an offset field in ELF file. Must be:
 *    - Elf64_Off for 32-bit CPU, or
 *    - Elf64_Off for 64-bit CPU.
 */
template <typename Elf_Addr, typename Elf_Off>
class ElfFileImpl : protected ElfFile {
/* Instance of this class must be instantiated from
 * ElfFile::Create() method only. */
friend class ElfFile;
 protected:
  /* Constructs ElfFileImpl instance. */
  ElfFileImpl() {
  };

  /* Destructs ElfFileImpl instance. */
  ~ElfFileImpl() {
  }

 protected:
  /* Initializes instance. This is an override of the base class method.
   * See ElfFile::initialize().
   */
  bool initialize(const Elf_CommonHdr* elf_hdr, const char* path);

  /* Parses DWARF, and buids list of compilation units for this ELF file.
   * This is an implementation of the base class' abstract method.
   * See ElfFile::parse_compilation_units().
   */
  virtual int parse_compilation_units(const DwarfParseContext* parse_context);

  /* Gets section information by section name.
   * Param:
   *  name - Name of the section to get information for.
   *  offset - Upon success contains offset of the section data in ELF file.
   *  size - Upon success contains size of the section data in ELF file.
   * Return:
   *  true on sucess, or false if section with such name doesn't exist in
   *  this ELF file.
   */
  bool get_section_info_by_name(const char* name,
                                Elf_Off* offset,
                                Elf_Word* size);

  /* Maps section by its name.
   * Param:
   *  name - Name of the section to map.
   *  section - Upon success contains section's mapping information.
   * Return:
   *  true on sucess, or false if section with such name doesn't exist in
   *  this ELF file, or mapping has failed.
   */
  bool map_section_by_name(const char* name, ElfMappedSection* section);
};

#endif  // ELFF_ELF_FILE_H_
