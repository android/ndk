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
 * Contains implementation of ElfFile classes that encapsulate an ELF file.
 */

#include "string.h"
#include "elf_file.h"
#include "elf_alloc.h"
#include "dwarf_cu.h"
#include "dwarf_utils.h"

#include <fcntl.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif

/* Tags to parse when collecting info about routines. */
static const Dwarf_Tag parse_rt_tags[] = {
  DW_TAG_compile_unit,
  DW_TAG_partial_unit,
  DW_TAG_inlined_subroutine,
  DW_TAG_subprogram,
  0
};
static const DwarfParseContext parse_rt_context = { parse_rt_tags };

//=============================================================================
// Base ElfFile implementation
//=============================================================================

ElfFile::ElfFile()
    : fixed_base_address_(0),
      elf_handle_((MapFile*)-1),
      elf_file_path_(NULL),
      allocator_(NULL),
      sec_table_(NULL),
      sec_count_(0),
      sec_entry_size_(0),
      last_cu_(NULL),
      cu_count_(0),
      is_exec_(0) {
}

ElfFile::~ElfFile() {
  DwarfCU* cu_to_del = last_cu_;
  while (cu_to_del != NULL) {
    DwarfCU* next_cu_to_del = cu_to_del->prev_cu_;
    delete cu_to_del;
    cu_to_del = next_cu_to_del;
  }

  if (mapfile_is_valid(elf_handle_)) {
    mapfile_close(elf_handle_);
  }

  if (elf_file_path_ != NULL) {
    delete[] elf_file_path_;
  }

  if (sec_table_ != NULL) {
    delete[] reinterpret_cast<Elf_Byte*>(sec_table_);
  }

  /* Must be deleted last! */
  if (allocator_ != NULL) {
    delete allocator_;
  }
}

ElfFile* ElfFile::Create(const char* path) {
  ElfFile* ret = NULL;
  /* Allocate enough space on the stack to fit the largest ELF file header. */
  Elf64_FHdr header;
  const Elf_CommonHdr* elf_hdr = &header.common;

  assert(path != NULL && *path != '\0');
  if (path == NULL || *path == '\0') {
    _set_errno(EINVAL);
    return NULL;
  }

  /*
   * Open ELF file, and read its header (the largest one possible).
   */
  MapFile* file_handle = mapfile_open(path, O_RDONLY | O_BINARY, 0);
  if (!mapfile_is_valid(file_handle)) {
    return NULL;
  }
  const ssize_t read_bytes = mapfile_read(file_handle, &header, sizeof(header));
  mapfile_close(file_handle);
  assert(read_bytes != -1 && read_bytes == sizeof(header));
  if (read_bytes == -1 || read_bytes != sizeof(header)) {
    if (read_bytes != -1) {
      _set_errno(EINVAL);
    }
    return NULL;
  }

  /* Lets see if this is an ELF file at all. */
  if (memcmp(elf_hdr->e_ident, ELFMAG, SELFMAG) != 0) {
    /* File is not an ELF file. */
    _set_errno(ENOEXEC);
    return NULL;
  }

  /* Lets check ELF's "bitness". */
  assert(elf_hdr->ei_info.ei_class == ELFCLASS32 ||
         elf_hdr->ei_info.ei_class == ELFCLASS64);
  if (elf_hdr->ei_info.ei_class != ELFCLASS32 &&
      elf_hdr->ei_info.ei_class != ELFCLASS64) {
    /* Neither 32, or 64-bit ELF file. Something wrong here. */
    _set_errno(EBADF);
    return NULL;
  }

  /* Lets instantiate appropriate ElfFileImpl object for this ELF. */
  if (elf_hdr->ei_info.ei_class == ELFCLASS32) {
    ret = new ElfFileImpl<Elf32_Addr, Elf32_Off>;
  } else {
    ret = new ElfFileImpl<Elf64_Addr, Elf64_Off>;
  }
  assert(ret != NULL);
  if (ret != NULL) {
    if (!ret->initialize(elf_hdr, path)) {
      delete ret;
      ret = NULL;
    }
  } else {
    _set_errno(ENOMEM);
  }

  return ret;
}

bool ElfFile::initialize(const Elf_CommonHdr* elf_hdr, const char* path) {
  /* Must be created first! */
  allocator_ = new ElfAllocator();
  assert(allocator_ != NULL);
  if (allocator_ == NULL) {
    _set_errno(ENOMEM);
    return false;
  }

  /* Copy file path. */
  size_t path_len = strlen(path) + 1;
  elf_file_path_ = new char[path_len];
  assert(elf_file_path_ != NULL);
  if (elf_file_path_ == NULL) {
    _set_errno(ENOMEM);
    return false;
  }
  memcpy(elf_file_path_, path, path_len);

  /* Cache some basic ELF properties. */
  is_ELF_64_ = elf_hdr->ei_info.ei_class == ELFCLASS64;
  is_elf_big_endian_ = elf_hdr->ei_info.ei_data == ELFDATA2MSB;
  same_endianness_ = is_elf_little_endian() == is_little_endian_cpu();
  is_exec_ = elf_hdr->e_type == 2;

  /* Reopen file for further reads and mappings. */
  elf_handle_ = mapfile_open(elf_file_path_, O_RDONLY | O_BINARY, 0);
  return mapfile_is_valid(elf_handle_);
}

bool ElfFile::get_pc_address_info(Elf_Xword address,
                                  Elf_AddressInfo* address_info) {
  assert(address_info != NULL);
  if (address_info == NULL) {
    _set_errno(EINVAL);
    return false;
  }

  /* Collect routine information for all CUs in this file. */
  if (parse_compilation_units(&parse_rt_context) == -1) {
    return false;
  }

  /* Iterate through the collected CUs looking for the one that
   * contains the given address. */
  address_info->inline_stack = NULL;
  DwarfCU* cu = last_cu();
  while (cu != NULL) {
    /* Find a leaf DIE object in the current CU that contains the address. */
    Dwarf_AddressInfo info;
    info.die_obj = cu->get_leaf_die_for_address(address);
    if (info.die_obj != NULL) {
      /* Convert the address to a location inside source file. */
      if (cu->get_pc_address_file_info(address, &info)) {
          /* Copy location information to the returning structure. */
          address_info->file_name = info.file_name;
          address_info->dir_name = info.dir_name;
          address_info->line_number = info.line_number;
      } else {
          address_info->file_name = NULL;
          address_info->dir_name = NULL;
          address_info->line_number = 0;
      }

      /* Lets see if the DIE represents a routine (rather than
       * a lexical block, for instance). */
      Dwarf_Tag tag = info.die_obj->get_tag();
      while (!dwarf_tag_is_routine(tag)) {
        /* This is not a routine DIE. Lets loop trhough the parents of that
         * DIE looking for the first routine DIE. */
        info.die_obj = info.die_obj->parent_die();
        if (info.die_obj == NULL) {
          /* Reached compilation unit DIE. Can't go any further. */
          address_info->routine_name = "<unknown>";
          return true;
        }
        tag = info.die_obj->get_tag();
      }

      /* Save name of the routine that contains the address. */
      address_info->routine_name = info.die_obj->get_name();
      if (address_info->routine_name == NULL) {
        /* In some cases (minimum debugging info in the file) routine
         * name may be not avaible. We, however, are obliged by API
         * considerations to return something in this field. */
          address_info->routine_name = "<unknown>";
      }

      /* Lets see if address belongs to an inlined routine. */
      if (tag != DW_TAG_inlined_subroutine) {
        address_info->inline_stack = NULL;
        return true;
      }

      /*
       * Address belongs to an inlined routine. Create inline stack.
       */

      /* Allocate inline stack array big enough to fit all parent entries. */
      address_info->inline_stack =
        new Elf_InlineInfo[info.die_obj->get_level() + 1];
      assert(address_info->inline_stack != NULL);
      if (address_info->inline_stack == NULL) {
        _set_errno(ENOMEM);
        return false;
      }
      memset(address_info->inline_stack, 0,
             sizeof(Elf_InlineInfo) * (info.die_obj->get_level() + 1));

      /* Reverse DIEs filling in inline stack entries for inline
       * routine tags. */
      int inl_index = 0;
      do {
        /* Save source file information. */
        DIEAttrib file_desc;
        if (info.die_obj->get_attrib(DW_AT_call_file, &file_desc)) {
          const Dwarf_STMTL_FileDesc* desc =
              cu->get_stmt_file_info(file_desc.value()->u32);
          if (desc != NULL) {
            address_info->inline_stack[inl_index].inlined_in_file =
                desc->file_name;
            address_info->inline_stack[inl_index].inlined_in_file_dir =
                cu->get_stmt_dir_name(desc->get_dir_index());
          }
        }
        if (address_info->inline_stack[inl_index].inlined_in_file == NULL) {
          address_info->inline_stack[inl_index].inlined_in_file = "<unknown>";
          address_info->inline_stack[inl_index].inlined_in_file_dir = NULL;
        }

        /* Save source line information. */
        if (info.die_obj->get_attrib(DW_AT_call_line, &file_desc)) {
          address_info->inline_stack[inl_index].inlined_at_line = file_desc.value()->u32;
        }

        /* Advance DIE to the parent routine, and save its name. */
        info.die_obj = info.die_obj->parent_die();
        assert(info.die_obj != NULL);
        if (info.die_obj != NULL) {
          tag = info.die_obj->get_tag();
          while (!dwarf_tag_is_routine(tag)) {
            info.die_obj = info.die_obj->parent_die();
            if (info.die_obj == NULL) {
              break;
            }
            tag = info.die_obj->get_tag();
          }
          if (info.die_obj != NULL) {
            address_info->inline_stack[inl_index].routine_name =
                info.die_obj->get_name();
          }
        }
        if (address_info->inline_stack[inl_index].routine_name == NULL) {
          address_info->inline_stack[inl_index].routine_name = "<unknown>";
        }

        /* Continue with the parent DIE. */
        inl_index++;
      } while (info.die_obj != NULL && tag == DW_TAG_inlined_subroutine);

      return true;
    }
    cu = cu->prev_cu();
  }

  return false;
}

void ElfFile::free_pc_address_info(Elf_AddressInfo* address_info) const {
  assert(address_info != NULL);
  if (address_info != NULL && address_info->inline_stack != NULL) {
    delete address_info->inline_stack;
    address_info->inline_stack = NULL;
  }
}

//=============================================================================
// ElfFileImpl
//=============================================================================

template <typename Elf_Addr, typename Elf_Off>
bool ElfFileImpl<Elf_Addr, Elf_Off>::initialize(const Elf_CommonHdr* elf_hdr,
                                                const char* path) {
  /* Must be called first! */
  if (!ElfFile::initialize(elf_hdr, path)) {
    return false;
  }

  /* Cache some header data, so later we can discard the header. */
  const Elf_FHdr<Elf_Addr, Elf_Off>* header =
      reinterpret_cast<const Elf_FHdr<Elf_Addr, Elf_Off>*>(elf_hdr);
  sec_count_ = pull_val(header->e_shnum);
  sec_entry_size_ = pull_val(header->e_shentsize);
  fixed_base_address_ = pull_val(header->e_entry) & ~0xFFF;

  /* Cache section table (must have one!) */
  const Elf_Off sec_table_off = pull_val(header->e_shoff);
  assert(sec_table_off != 0 && sec_count_ != 0);
  if (sec_table_off == 0 || sec_count_ == 0) {
    _set_errno(EBADF);
    return false;
  }
  const size_t sec_table_size = sec_count_ * sec_entry_size_;
  sec_table_ = new Elf_Byte[sec_table_size];
  assert(sec_table_ != NULL);
  if (sec_table_ == NULL) {
    _set_errno(ENOMEM);
    return false;
  }
  if (mapfile_read_at(elf_handle_, sec_table_off, sec_table_,
                      sec_table_size) < 0) {
      return false;
  }

  /* Map ELF's string section (must have one!). */
  const Elf_Half str_sec_index = pull_val(header->e_shstrndx);
  assert(str_sec_index != SHN_UNDEF);
  if (str_sec_index == SHN_UNDEF) {
    _set_errno(EBADF);
    return false;
  }
  const Elf_SHdr<Elf_Addr, Elf_Off>* str_sec =
      reinterpret_cast<const Elf_SHdr<Elf_Addr, Elf_Off>*>
          (get_section_by_index(str_sec_index));
  assert(str_sec != NULL);
  if (str_sec == NULL) {
    _set_errno(EBADF);
    return false;
  }
  if (!string_section_.map(elf_handle_, pull_val(str_sec->sh_offset),
                           pull_val(str_sec->sh_size))) {
    return false;
  }

  /* Lets determine DWARF format. According to the docs, DWARF is 64 bit, if
   * first 4 bytes in the compilation unit header are set to 0xFFFFFFFF.
   * .debug_info section of the ELF file begins with the first CU header. */
  if (!map_section_by_name(".debug_info", &debug_info_)) {
    _set_errno(EBADF);
    return false;
  }

  /* Note that we don't care about endianness here, since 0xFFFFFFFF is an
   * endianness-independent value, so we don't have to pull_val here. */
  is_DWARF_64_ =
    *reinterpret_cast<const Elf_Word*>(debug_info_.data()) == 0xFFFFFFFF;

  return true;
}

template <typename Elf_Addr, typename Elf_Off>
int ElfFileImpl<Elf_Addr, Elf_Off>::parse_compilation_units(
    const DwarfParseContext* parse_context) {
  /* Lets see if we already parsed the file. */
  if (last_cu() != NULL) {
    return cu_count_;
  }

  /* Cache sections required for this parsing. */
  if (!map_section_by_name(".debug_abbrev", &debug_abbrev_) ||
      !map_section_by_name(".debug_ranges", &debug_ranges_) ||
      !map_section_by_name(".debug_line", &debug_line_) ||
      !map_section_by_name(".debug_str", &debug_str_)) {
    _set_errno(EBADF);
    return false;
  }

  /* .debug_info section opens with the first CU header. */
  const void* next_cu = debug_info_.data();

  /* Iterate through CUs until we reached the end of .debug_info section, or
   * advanced to a CU with zero size, indicating the end of CU list for this
   * file. */
  while (is_valid_cu(next_cu)) {
    /* Instatiate CU, depending on DWARF "bitness". */
    DwarfCU* cu = DwarfCU::create_instance(this, next_cu);
    if (cu == NULL) {
      _set_errno(ENOMEM);
      return -1;
    }

    if (cu->parse(parse_context, &next_cu)) {
      cu->set_prev_cu(last_cu_);
      last_cu_ = cu;
      cu_count_++;
    } else {
      delete cu;
      return -1;
    }
  };

  return cu_count_;
}

template <typename Elf_Addr, typename Elf_Off>
bool ElfFileImpl<Elf_Addr, Elf_Off>::get_section_info_by_name(const char* name,
                                                              Elf_Off* offset,
                                                              Elf_Word* size) {
  const Elf_SHdr<Elf_Addr, Elf_Off>* cur_section =
      reinterpret_cast<const Elf_SHdr<Elf_Addr, Elf_Off>*>(sec_table_);

  for (Elf_Half sec = 0; sec < sec_count_; sec++) {
    const char* sec_name = get_str_sec_str(pull_val(cur_section->sh_name));
    if (sec_name != NULL && strcmp(name, sec_name) == 0) {
      *offset = pull_val(cur_section->sh_offset);
      *size = pull_val(cur_section->sh_size);
      return true;
    }
    cur_section = reinterpret_cast<const Elf_SHdr<Elf_Addr, Elf_Off>*>
                                  (INC_CPTR(cur_section, sec_entry_size_));
  }
  _set_errno(EINVAL);
  return false;
}

template <typename Elf_Addr, typename Elf_Off>
bool ElfFileImpl<Elf_Addr, Elf_Off>::map_section_by_name(
    const char* name,
    ElfMappedSection* section) {
  if (section->is_mapped()) {
    return true;
  }

  Elf_Off offset;
  Elf_Word size;
  if (!get_section_info_by_name(name, &offset, &size)) {
    return false;
  }

  return section->map(elf_handle_, offset, size);
}
