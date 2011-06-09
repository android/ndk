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
 * Contains implementation of a class DwarfCU, that encapsulates a compilation
 * unit in the .debug_info section of the mapped ELF file.
 */

#include "string.h"
#include "stdio.h"
#include "elf_file.h"
#include "dwarf_cu.h"
#include "dwarf_utils.h"

DwarfCU::DwarfCU(ElfFile* elf)
    : elf_file_(elf),
      cu_die_(NULL),
      prev_cu_(NULL) {
}

DwarfCU::~DwarfCU() {
  if (cu_die_ != NULL) {
    delete cu_die_;
  }
  abbrs_.empty();
}

DwarfCU* DwarfCU::create_instance(ElfFile* elf, const void* hdr) {
  DwarfCU* ret;

  /* 64-bit DWARF CU has first 4 bytes in its header set to 0xFFFFFFFF. */
  if (*reinterpret_cast<const Elf_Word*>(hdr) == 0xFFFFFFFF) {
    ret = new(elf) DwarfCUImpl<Dwarf64_CUHdr, Dwarf64_Off>
                      (elf, reinterpret_cast<const Dwarf64_CUHdr*>(hdr));
  } else {
    ret = new(elf) DwarfCUImpl<Dwarf32_CUHdr, Dwarf32_Off>
                      (elf, reinterpret_cast<const Dwarf32_CUHdr*>(hdr));
  }
  assert(ret != NULL);
  if (ret == NULL) {
    _set_errno(ENOMEM);
  }
  return ret;
}

const Elf_Byte* DwarfCU::process_attrib(const Elf_Byte* prop,
                                        Dwarf_Form form,
                                        Dwarf_Value* attr_value) const {
  assert(form != 0);
  Dwarf_Value tmp_val;
  Dwarf_Value leb128;

  attr_value->type = DWARF_VALUE_UNKNOWN;
  attr_value->encoded_size = 0;
  attr_value->u64 = 0;

  switch (form) {
    /* Property is a block of data, contained in .debug_info section. Block
     * size is encoded with 1 byte value, and block data immediately follows
     * block size. */
    case DW_FORM_block1:
      attr_value->type = DWARF_VALUE_BLOCK;
      attr_value->block.block_size = *prop;
      attr_value->block.block_ptr = prop + 1;
      attr_value->encoded_size =
          static_cast<Elf_Word>(attr_value->block.block_size + 1);
      break;

    /* Property is a block of data, contained in .debug_info section. Block
     * size is encoded with 2 bytes value, and block data immediately follows
     * block size. */
    case DW_FORM_block2:
      attr_value->type = DWARF_VALUE_BLOCK;
      attr_value->block.block_size =
          elf_file_->pull_val(reinterpret_cast<const Elf_Half*>(prop));
      attr_value->block.block_ptr = prop + 2;
      attr_value->encoded_size =
          static_cast<Elf_Word>(attr_value->block.block_size + 2);
      break;

    /* Property is a block of data, contained in .debug_info section. Block
     * size is encoded with 4 bytes value, and block data immediately follows
     * block size. */
    case DW_FORM_block4:
      attr_value->type = DWARF_VALUE_BLOCK;
      attr_value->block.block_size =
          elf_file_->pull_val(reinterpret_cast<const Elf_Word*>(prop));
      attr_value->block.block_ptr = prop + 4;
      attr_value->encoded_size =
          static_cast<Elf_Word>(attr_value->block.block_size + 4);
      break;

    /* Property is a block of data, contained in .debug_info section. Block
     * size is encoded with unsigned LEB128 value, and block data immediately
     * follows block size. */
    case DW_FORM_block:
      reinterpret_cast<const Dwarf_Leb128*>(prop)->process_unsigned(&leb128);
      attr_value->type = DWARF_VALUE_BLOCK;
      attr_value->block.block_size = leb128.u32;
      attr_value->block.block_ptr = prop + leb128.encoded_size;
      attr_value->encoded_size =
          static_cast<Elf_Word>(attr_value->block.block_size +
                                leb128.encoded_size);
      break;

    /* Property is unsigned 1 byte value. */
    case DW_FORM_flag:
    case DW_FORM_data1:
    case DW_FORM_ref1:
      attr_value->type = DWARF_VALUE_U8;
      attr_value->u8 = *prop;
      attr_value->encoded_size = 1;
      break;

    /* Property is unsigned 2 bytes value. */
    case DW_FORM_data2:
    case DW_FORM_ref2:
      attr_value->type = DWARF_VALUE_U16;
      attr_value->u16 =
          elf_file_->pull_val(reinterpret_cast<const Elf_Half*>(prop));
      attr_value->encoded_size = 2;
      break;

    /* Property is unsigned 4 bytes value. */
    case DW_FORM_data4:
    case DW_FORM_ref4:
      attr_value->type = DWARF_VALUE_U32;
      attr_value->u32 =
          elf_file_->pull_val(reinterpret_cast<const Elf_Word*>(prop));
      attr_value->encoded_size = 4;
      break;

    /* Property is unsigned 8 bytes value. */
    case DW_FORM_data8:
    case DW_FORM_ref8:
    case DW_FORM_ref_sig8:
      attr_value->type = DWARF_VALUE_U64;
      attr_value->u64 =
          elf_file_->pull_val(reinterpret_cast<const Elf_Xword*>(prop));
      attr_value->encoded_size = 8;
      break;

    /* Property is signed LEB128 value. */
    case DW_FORM_sdata:
      reinterpret_cast<const Dwarf_Leb128*>(prop)->process_signed(attr_value);
      break;

    /* Property is unsigned LEB128 value. */
    case DW_FORM_ref_udata:
    case DW_FORM_udata:
      reinterpret_cast<const Dwarf_Leb128*>(prop)->process_unsigned(attr_value);
      break;

    /* Property is a string contained directly in .debug_info section. */
    case DW_FORM_string:
      attr_value->type = DWARF_VALUE_STR;
      attr_value->str = reinterpret_cast<const char*>(prop);
      attr_value->encoded_size = strlen(attr_value->str) + 1;
      break;

    /* Property is an offset of a string contained in .debug_str section.
     * We will process the reference here, converting it into the actual
     * string value. */
    case DW_FORM_strp:
      attr_value->type = DWARF_VALUE_STR;
      if (elf_file_->is_DWARF_64()) {
        Elf_Xword str_offset =
            elf_file_->pull_val(reinterpret_cast<const Elf_Xword*>(prop));
        attr_value->str = elf_file_->get_debug_str(str_offset);
        attr_value->encoded_size = 8;
      } else {
        Elf_Word str_offset =
            elf_file_->pull_val(reinterpret_cast<const Elf_Word*>(prop));
        attr_value->str = elf_file_->get_debug_str(str_offset);
        attr_value->encoded_size = 4;
      }
      break;

    /* Property is an address. */
    case DW_FORM_addr:
      if (addr_sizeof_ == 4) {
        attr_value->type = DWARF_VALUE_PTR32;
        attr_value->u32 =
            elf_file_->pull_val(reinterpret_cast<const Elf_Word*>(prop));
      } else {
        attr_value->type = DWARF_VALUE_PTR64;
        attr_value->u64 =
            elf_file_->pull_val(reinterpret_cast<const Elf_Xword*>(prop));
      }
      attr_value->encoded_size = addr_sizeof_;
      break;

    /* Reference from the beginning of .debug_info section. */
    case DW_FORM_ref_addr:
      /* DWARF3+ requires that encoding size of this property must be 4 bytes
       * in 32-bit DWARF, and 8 bytes in 64-bit DWARF, while DWARF2- requires
       * encoding size to be equal to CU's pointer size. */
      if (is_DWARF3_or_higher()) {
        if (elf_file_->is_DWARF_64()) {
          attr_value->type = DWARF_VALUE_U64;
          attr_value->u64 =
              elf_file_->pull_val(reinterpret_cast<const Elf_Xword*>(prop));
          attr_value->encoded_size = 4;
        } else {
          attr_value->type = DWARF_VALUE_U32;
          attr_value->u32 =
              elf_file_->pull_val(reinterpret_cast<const Elf_Word*>(prop));
          attr_value->encoded_size = 8;
        }
      } else {
        if (addr_sizeof_ == 4) {
          attr_value->type = DWARF_VALUE_U32;
          attr_value->u32 =
              elf_file_->pull_val(reinterpret_cast<const Elf_Word*>(prop));
        } else {
          attr_value->type = DWARF_VALUE_U64;
          attr_value->u64 =
              elf_file_->pull_val(reinterpret_cast<const Elf_Xword*>(prop));
        }
        attr_value->encoded_size = addr_sizeof_;
      }
      break;

    /* Reference to a section, other than .debug_info, or .debug_str */
    case DW_FORM_sec_offset:
      if (elf_file_->is_DWARF_64()) {
        attr_value->type = DWARF_VALUE_U64;
        attr_value->u64 =
            elf_file_->pull_val(reinterpret_cast<const Elf_Xword*>(prop));
        attr_value->encoded_size = 4;
      } else {
        attr_value->type = DWARF_VALUE_U32;
        attr_value->u32 =
            elf_file_->pull_val(reinterpret_cast<const Elf_Word*>(prop));
        attr_value->encoded_size = 8;
      }
      break;

    /* This is a replacement for DW_FORM_flag, which doesn't consume memory
     * in .debug_info section, and only by the fact of its existence it is
     * equal to DW_FORM_flag with value set to 1. */
    case DW_FORM_flag_present:
      attr_value->type = DWARF_VALUE_U8;
      attr_value->u8 = 1;
      attr_value->encoded_size = 0;
      break;

    /* Encodes the actual form to be used. */
    case DW_FORM_indirect:
      // Starts with ULEB128
      prop = reinterpret_cast<const Elf_Byte*>
                (reinterpret_cast<const Dwarf_Leb128*>
                    (prop)->process_unsigned(&tmp_val));
      /* ULEB128 encodes the actual form to be used to process this entry. */
      process_attrib(prop, tmp_val.u16, attr_value);
      attr_value->encoded_size += tmp_val.encoded_size;
      break;

    /* This form is defined for DWARF4, and has no documentation whatsoever. */
    case DW_FORM_exprloc:
    default:
      attr_value->type = DWARF_VALUE_U32;
      attr_value->u32 =
          elf_file_->pull_val(reinterpret_cast<const Elf_Word*>(prop));
      attr_value->encoded_size = 4;
      break;
  }

  return prop + attr_value->encoded_size;
}

void DwarfCU::dump() const {
  printf("\n\n>>>>>>>>>>>>>>> CU %p (version %u, address size %u)\n",
         cu_die_->die(), static_cast<Elf_Word>(version_),
         static_cast<Elf_Word>(addr_sizeof_));
  printf(">>>>> Build dir path:  %s\n", comp_dir_path());
  printf(">>>>> Build file path: %s\n", rel_cu_path());
  if (cu_die_ != NULL) {
    cu_die_->dump(false);
  }
}

//=============================================================================
// DwarfCUImpl implementation
//=============================================================================

template <typename Dwarf_CUHdr, typename Dwarf_Off>
DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::DwarfCUImpl(ElfFile* elf,
                                                 const Dwarf_CUHdr* hdr)
    : DwarfCU(elf),
      cu_header_(hdr) {
  /* Cache CU's DIE abbreviation descriptor in the array. This MUST be done
   * BEFORE first call to array's cache_to() method. */
  const Dwarf_Abbr_DIE* cu_abbr_die = reinterpret_cast<const Dwarf_Abbr_DIE*>
                                 (INC_CPTR(elf->get_debug_abbrev_data(),
                                           elf->pull_val(hdr->abbrev_offset)));
  abbrs_.add(cu_abbr_die);

  cu_size_ = elf->pull_val(hdr->size_hdr.size);
  version_ = elf->pull_val(hdr->version);
  addr_sizeof_ = hdr->address_size;
  memset(&stmtl_header_, 0, sizeof(stmtl_header_));
}

template <typename Dwarf_CUHdr, typename Dwarf_Off>
bool DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::parse(
    const DwarfParseContext* parse_context,
    const void** next_cu_die) {
  /* Start parsing with the DIE for this CU. */
  if (process_DIE(parse_context, get_DIE(), NULL) == NULL) {
    return false;
  }

  /* CU area size (thus, next CU header offset) in .debug_info section equals
   * to CU size, plus number of bytes, required to encode CU size in CU header
   * (4 for 32-bit CU, and 12 for 64-bit CU. */
  *next_cu_die =
      INC_CPTR(cu_header_, cu_size_ + ELFF_FIELD_OFFSET(Dwarf_CUHdr, version));

  return true;
}

template <typename Dwarf_CUHdr, typename Dwarf_Off>
const Elf_Byte* DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::process_DIE(
    const DwarfParseContext* parse_context,
    const Dwarf_DIE* die,
    DIEObject* parent_obj) {
  while (is_attrib_ptr_valid(die) && !die->is_separator()) {
    Dwarf_AbbrNum abbr_num;
    Dwarf_Tag     die_tag;
    Elf_Word      sibling_off = 0;

    /* Get DIE's abbreviation number, and advance to DIE's properties. */
    const Elf_Byte* die_attr = die->process(&abbr_num);

    /* Get abbreviation for the current DIE. */
    const Dwarf_Abbr_DIE* die_abbr = abbrs_.cache_to(abbr_num);
    if (die_abbr == NULL) {
      return NULL;
    }

    /* Get base DIE properties, and advance to the DIE's
     * attribute descriptors. */
    const Dwarf_Abbr_AT* at_abbr = die_abbr->process(NULL, &die_tag);

    /* Instantiate DIE object for this DIE, and get list of properties,
     * that should be collected while processing that DIE. */
    DIEObject* die_obj =
      create_die_object(parse_context, die, parent_obj, die_tag);
    if (die_obj == NULL && errno != 0) {
      return NULL;
    }

    if (die_obj != NULL) {
      if (parent_obj != NULL) {
        /* Update list of parent's children. */
        die_obj->link_sibling(parent_obj->last_child());
        parent_obj->link_child(die_obj);
      } else {
        /* NULL parent object is allowed only for CU DIE itself. */
        assert(cu_die_ == NULL && die_tag == DW_TAG_compile_unit);
        if (cu_die_ == NULL && die_tag != DW_TAG_compile_unit) {
          _set_errno(EINVAL);
          return NULL;
        }
        cu_die_ = die_obj;
        /* This CU DIE object will be used as a parent for all DIE
         * objects, created in this method. */
        parent_obj = cu_die_;
      }
    }

    // Loop through all DIE properties.
    while (elf_file_->is_valid_abbr_ptr(at_abbr, sizeof(Dwarf_Abbr_AT)) &&
           !at_abbr->is_separator()) {
      Dwarf_At    at_value;
      Dwarf_Form  at_form;
      Dwarf_Value attr_value;

      // Obtain next property value.
      at_abbr = at_abbr->process(&at_value, &at_form);
      die_attr = process_attrib(die_attr, at_form, &attr_value);

      if (at_value == DW_AT_sibling) {
        /* DW_AT_sibling means that next DIE is a child of the one that's
         * being currently processed. We need to cache value of this property
         * in order to correctly calculate next sibling of this DIE after
         * child's DIE has been processed. */
        assert(sibling_off == 0);
        sibling_off = attr_value.u32;
      }
    }

    /* Next DIE immediately follows last property for the current DIE. */
    die = reinterpret_cast<const Dwarf_DIE*>(die_attr);
    if (sibling_off != 0) {
      // Process child DIE.
      process_DIE(parse_context, die, die_obj != NULL ? die_obj : parent_obj);
      // Next sibling DIE offset is relative to this CU's header beginning.
      die = INC_CPTR_T(Dwarf_DIE, cu_header_, sibling_off);
    }
  }

  return INC_CPTR_T(Elf_Byte, die, 1);
}

template <typename Dwarf_CUHdr, typename Dwarf_Off>
DIEObject* DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::create_die_object(
    const DwarfParseContext* parse_context,
    const Dwarf_DIE* die,
    DIEObject* parent,
    Dwarf_Tag tag) {
  DIEObject* ret = NULL;

  /* We will always create a DIE object for CU DIE. */
  if (tag == DW_TAG_compile_unit || collect_die(parse_context, tag)) {
    ret = new(elf_file_) DIEObject(die, this, parent);
    assert(ret != NULL);
    if (ret == NULL) {
      _set_errno(ENOMEM);
    }
  } else {
    _set_errno(0);
  }
  return ret;
}

template <typename Dwarf_CUHdr, typename Dwarf_Off>
bool DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::init_stmtl() {
  if (stmtl_header_.unit_length != 0) {
    return true;
  }

  assert(cu_die_ != NULL);
  if (cu_die_ == NULL) {
    _set_errno(EINVAL);
    return false;
  }

  DIEAttrib stmtl;
  if (!cu_die()->get_attrib(DW_AT_stmt_list, &stmtl)) {
    _set_errno(EINVAL);
    return false;
  }

  const void* stmtl_start =
      INC_CPTR(elf_file()->get_debug_line_data(), stmtl.value()->u32);
  if (*reinterpret_cast<const Elf_Word*>(stmtl_start) == 0xFFFFFFFF) {
    cache_stmtl<Dwarf64_STMTLHdr>(reinterpret_cast<const Dwarf64_STMTLHdr*>(stmtl_start));
  } else {
    cache_stmtl<Dwarf32_STMTLHdr>(reinterpret_cast<const Dwarf32_STMTLHdr*>(stmtl_start));
  }

  return true;
}

template <typename Dwarf_CUHdr, typename Dwarf_Off>
bool DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::get_pc_address_file_info(
    Elf_Xword address,
    Dwarf_AddressInfo* info) {
  /* Make sure STMTL header is cached. */
  if (!init_stmtl()) {
    return false;
  }
  /* Flags address match, that should trigger return next time
   * source line gets adjusted. */
  bool found = false;
  /* Create new state machine. */
  DwarfStateMachine state(stmtl_header_.default_is_stmt != 0);

  /* Start the "Line Number Program" */
  const Elf_Byte* go = stmtl_header_.start;
  while (go < stmtl_header_.end) {
    const Elf_Byte op = *go;
    go++;

    if (op == 0) {
      /* This is an extended opcode. */
      Dwarf_Value op_size;

      /* First ULEB128 contains opcode size, (excluding ULEB128 itself). */
      go = reinterpret_cast<const Elf_Byte*>
             (reinterpret_cast<const Dwarf_Leb128*>(go)->process_unsigned(&op_size));
      /* Next is the extended opcode. */
      const Elf_Byte* ex_op_ptr = go;
      switch (*ex_op_ptr) {
        case DW_LNE_end_sequence:
          state.end_sequence_ = true;
          state.reset(stmtl_header_.default_is_stmt != 0);
          found = false;
          break;

        case DW_LNE_set_address: {
          Elf_Xword prev_address = state.address_;
          if (is_CU_address_64()) {
            state.address_ =
              elf_file()->pull_val(reinterpret_cast<const Elf_Xword*>(ex_op_ptr + 1));
          } else {
            state.address_ =
              elf_file()->pull_val(reinterpret_cast<const Elf_Word*>(ex_op_ptr + 1));
          }
          if (prev_address != 0 &&
              address >= prev_address && address < state.address_) {
            return set_source_info(&state, info);
          } else if (address == state.address_) {
            found = true;
          }
          break;
        }

        case DW_LNE_define_file: {
          /* Parameters start with the directly encoded zero-terminated
           * file name. */
          state.set_file_info_ = INC_CPTR_T(Dwarf_STMTL_FileDesc, ex_op_ptr, 1);
          assert(state.set_file_info_ != NULL);
          if (state.set_file_info_ != NULL) {
            ex_op_ptr = reinterpret_cast<const Elf_Byte*>(state.set_file_info_->process(NULL));
          }
          break;
        }

        case DW_LNE_set_discriminator: {
          Dwarf_Value discr_val;
          /* One parameter: discriminator's ULEB128 value. */
          reinterpret_cast<const Dwarf_Leb128*>(ex_op_ptr + 1)->process_unsigned(&discr_val);
          state.discriminator_ = discr_val.u32;
          break;
        }

        default:
          assert(0);
          return false;
      }
      go += op_size.u32;
    } else if (op < stmtl_header_.opcode_base) {
      /* This is a standard opcode. */
      switch (op) {
        case DW_LNS_copy:
          /* No parameters. */
          state.basic_block_ = false;
          state.prologue_end_ = false;
          state.epilogue_begin_ = false;
          break;

        case DW_LNS_advance_pc: {
          /* One parameter: ULEB128 value to add to the current address value
           * in the state machine. */
          Dwarf_Value addr_add;
          go = reinterpret_cast<const Elf_Byte*>
              (reinterpret_cast<const Dwarf_Leb128*>(go)->process_unsigned(&addr_add));
          Elf_Xword prev_address = state.address_;
          state.address_ += addr_add.u64;
          if (prev_address != 0 &&
              address >= prev_address && address < state.address_) {
            return set_source_info(&state, info);
          } else if (address == state.address_) {
            found = true;
          }
          break;
        }

        case DW_LNS_advance_line: {
          /* One parameter: signed LEB128 value to add to the current line
           * number in the state machine. */
          Dwarf_Value line_add;
          go = reinterpret_cast<const Elf_Byte*>
              (reinterpret_cast<const Dwarf_Leb128*>(go)->process_signed(&line_add));
          state.line_ += line_add.s32;
          if (found) {
            return set_source_info(&state, info);
          }
          break;
        }

        case DW_LNS_set_file: {
          /* One parameter: ULEB128 value encoding current file number. */
          Dwarf_Value file_num;
          go = reinterpret_cast<const Elf_Byte*>
              (reinterpret_cast<const Dwarf_Leb128*>(go)->process_unsigned(&file_num));
          state.file_ = file_num.u32;
          /* This operation should discard previously saved file information. */
          state.set_file_info_ = NULL;
          break;
        }

        case DW_LNS_set_column: {
          /* One parameter: ULEB128 value encoding current column number. */
          Dwarf_Value column_num;
          go = reinterpret_cast<const Elf_Byte*>
              (reinterpret_cast<const Dwarf_Leb128*>(go)->process_unsigned(&column_num));
          state.column_ = column_num.u32;
          break;
        }

        case DW_LNS_negate_stmt:
          /* No parameters. */
          state.is_stmt_ = !state.is_stmt_;
          break;

        case DW_LNS_set_basic_block:
          /* No parameters. */
          state.basic_block_ = true;
          break;

        case DW_LNS_const_add_pc: {
          Elf_Xword prev_address = state.address_;
          /* No parameters. This operation does the same thing, as special
           * opcode 255 would do to the current address. */
          Elf_Word adjusted =
              static_cast<Elf_Word>(255) - stmtl_header_.opcode_base;
          state.address_ += (adjusted / stmtl_header_.line_range) *
                            stmtl_header_.min_instruction_len;
          if (prev_address != 0 &&
              address >= prev_address && address < state.address_) {
            return set_source_info(&state, info);
          } else if (address == state.address_) {
            found = true;
          }
          break;
        }

        case DW_LNS_fixed_advance_pc: {
          Elf_Xword prev_address = state.address_;
          /* One parameter: directly encoded 16-bit value to add to the
           * current address. */
          state.address_ +=
              elf_file()->pull_val(reinterpret_cast<const Elf_Half*>(go));
          if (prev_address != 0 &&
              address >= prev_address && address < state.address_) {
            return set_source_info(&state, info);
          } else if (address == state.address_) {
            found = true;
          }
          go += sizeof(Elf_Half);
          break;
        }

        case DW_LNS_set_prologue_end:
          /* No parameters. */
          state.prologue_end_ = true;
          break;

        case DW_LNS_set_epilogue_begin:
          /* No parameters. */
          state.epilogue_begin_ = true;
          break;

        case DW_LNS_set_isa: {
          /* One parameter: ISA value encoded as ULEB128. */
          Dwarf_Value isa_val;
          go = reinterpret_cast<const Elf_Byte*>
              (reinterpret_cast<const Dwarf_Leb128*>(go)->process_unsigned(&isa_val));
          state.isa_ = isa_val.u32;
          break;
        }

        default:
          /* Unknown opcode. Just skip it. */
          for (Elf_Byte uleb = 0;
               uleb < stmtl_header_.standard_opcode_lengths[op - 1]; uleb++) {
            Dwarf_Value tmp;
            go = reinterpret_cast<const Elf_Byte*>
              (reinterpret_cast<const Dwarf_Leb128*>(go)->process_unsigned(&tmp));
          }
          break;
      }
    } else {
      Elf_Xword prev_address = state.address_;
      /* This is a special opcode. */
      const Elf_Word adjusted = op - stmtl_header_.opcode_base;
      /* Advance address. */
      state.address_ += (adjusted / stmtl_header_.line_range) *
                        stmtl_header_.min_instruction_len;
      if (prev_address != 0 &&
          address >= prev_address && address < state.address_) {
        return set_source_info(&state, info);
      }
      /* Advance line. */
      state.line_ += stmtl_header_.line_base +
                     (adjusted % stmtl_header_.line_range);
      if (state.address_ == address) {
        return set_source_info(&state, info);
      }
      /* Do the woodoo. */
      state.basic_block_ = false;
      state.prologue_end_ = false;
      state.epilogue_begin_ = false;
    }
  }

  return false;
}

template <typename Dwarf_CUHdr, typename Dwarf_Off>
const Dwarf_STMTL_FileDesc* DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::get_stmt_file_info(
    Elf_Word index) {
  /* Index must be 1-based. */
  if (index == 0) {
    return NULL;
  }

  const Dwarf_STMTL_FileDesc* cur_desc = stmtl_header_.file_infos;
  while (index != 1 && !cur_desc->is_last_entry()) {
    cur_desc = cur_desc->process(NULL);
    index--;
  }
  assert(!cur_desc->is_last_entry());
  return cur_desc->is_last_entry() ? NULL : cur_desc;
}

template <typename Dwarf_CUHdr, typename Dwarf_Off>
const char* DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::get_stmt_dir_name(
    Elf_Word dir_index) {
  if (dir_index == 0) {
    /* Requested is current compilation directory. */
    return comp_dir_path();
  }
  if (dir_index > stmtl_header_.inc_dir_num) {
    return NULL;
  }

  const char* cur_dir = stmtl_header_.include_directories;
  while (dir_index != 1) {
    cur_dir += strlen(cur_dir) + 1;
    dir_index--;
  }
  return cur_dir;
}

template <typename Dwarf_CUHdr, typename Dwarf_Off>
bool DwarfCUImpl<Dwarf_CUHdr, Dwarf_Off>::set_source_info(
    const DwarfStateMachine* state,
    Dwarf_AddressInfo* info) {
  info->line_number = state->line_;
  const Dwarf_STMTL_FileDesc* file_info = state->set_file_info_;
  if (file_info == NULL) {
    file_info = get_stmt_file_info(state->file_);
    if (file_info == NULL) {
      info->file_name = rel_cu_path();
      info->dir_name = comp_dir_path();
      return true;
    }
  }
  info->file_name = file_info->get_file_name();
  const Elf_Word dir_index = file_info->get_dir_index();
  info->dir_name = get_stmt_dir_name(dir_index);
  return true;
}

