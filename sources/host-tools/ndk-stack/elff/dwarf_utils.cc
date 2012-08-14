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
 * Contains implementation of misc. DWARF utility routines.
 */

#include <stdio.h>
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include "dwarf_utils.h"

/* "Stringifies" the parameter. */
#define DWARF_NAMEFY(val) case val: return "" #val ""

/* "Stringifies" two parameters. */
#define DWARF_NAMEFY2(val1, val2) case val1: return "" #val1 " | " #val2 ""

const char*
dwarf_at_name(Dwarf_At at) {
  switch (at) {
    DWARF_NAMEFY(DW_AT_sibling);
    DWARF_NAMEFY(DW_AT_location);
    DWARF_NAMEFY(DW_AT_name);
    DWARF_NAMEFY(DW_AT_ordering);
    DWARF_NAMEFY(DW_AT_subscr_data);
    DWARF_NAMEFY(DW_AT_byte_size);
    DWARF_NAMEFY(DW_AT_bit_offset);
    DWARF_NAMEFY(DW_AT_bit_size);
    DWARF_NAMEFY(DW_AT_element_list);
    DWARF_NAMEFY(DW_AT_stmt_list);
    DWARF_NAMEFY(DW_AT_low_pc);
    DWARF_NAMEFY(DW_AT_high_pc);
    DWARF_NAMEFY(DW_AT_language);
    DWARF_NAMEFY(DW_AT_member);
    DWARF_NAMEFY(DW_AT_discr);
    DWARF_NAMEFY(DW_AT_discr_value);
    DWARF_NAMEFY(DW_AT_visibility);
    DWARF_NAMEFY(DW_AT_import);
    DWARF_NAMEFY(DW_AT_string_length);
    DWARF_NAMEFY(DW_AT_common_reference);
    DWARF_NAMEFY(DW_AT_comp_dir);
    DWARF_NAMEFY(DW_AT_const_value);
    DWARF_NAMEFY(DW_AT_containing_type);
    DWARF_NAMEFY(DW_AT_default_value);
    DWARF_NAMEFY(DW_AT_inline);
    DWARF_NAMEFY(DW_AT_is_optional);
    DWARF_NAMEFY(DW_AT_lower_bound);
    DWARF_NAMEFY(DW_AT_producer);
    DWARF_NAMEFY(DW_AT_prototyped);
    DWARF_NAMEFY(DW_AT_return_addr);
    DWARF_NAMEFY(DW_AT_start_scope);
    DWARF_NAMEFY2(DW_AT_bit_stride, DW_AT_stride_size);
    DWARF_NAMEFY(DW_AT_upper_bound);
    DWARF_NAMEFY(DW_AT_abstract_origin);
    DWARF_NAMEFY(DW_AT_accessibility);
    DWARF_NAMEFY(DW_AT_address_class);
    DWARF_NAMEFY(DW_AT_artificial);
    DWARF_NAMEFY(DW_AT_base_types);
    DWARF_NAMEFY(DW_AT_calling_convention);
    DWARF_NAMEFY(DW_AT_count);
    DWARF_NAMEFY(DW_AT_data_member_location);
    DWARF_NAMEFY(DW_AT_decl_column);
    DWARF_NAMEFY(DW_AT_decl_file);
    DWARF_NAMEFY(DW_AT_decl_line);
    DWARF_NAMEFY(DW_AT_declaration);
    DWARF_NAMEFY(DW_AT_discr_list);
    DWARF_NAMEFY(DW_AT_encoding);
    DWARF_NAMEFY(DW_AT_external);
    DWARF_NAMEFY(DW_AT_frame_base);
    DWARF_NAMEFY(DW_AT_friend);
    DWARF_NAMEFY(DW_AT_identifier_case);
    DWARF_NAMEFY(DW_AT_macro_info);
    DWARF_NAMEFY(DW_AT_namelist_item);
    DWARF_NAMEFY(DW_AT_priority);
    DWARF_NAMEFY(DW_AT_segment);
    DWARF_NAMEFY(DW_AT_specification);
    DWARF_NAMEFY(DW_AT_static_link);
    DWARF_NAMEFY(DW_AT_type);
    DWARF_NAMEFY(DW_AT_use_location);
    DWARF_NAMEFY(DW_AT_variable_parameter);
    DWARF_NAMEFY(DW_AT_virtuality);
    DWARF_NAMEFY(DW_AT_vtable_elem_location);
    DWARF_NAMEFY(DW_AT_allocated);
    DWARF_NAMEFY(DW_AT_associated);
    DWARF_NAMEFY(DW_AT_data_location);
    DWARF_NAMEFY2(DW_AT_byte_stride, DW_AT_stride);
    DWARF_NAMEFY(DW_AT_entry_pc);
    DWARF_NAMEFY(DW_AT_use_UTF8);
    DWARF_NAMEFY(DW_AT_extension);
    DWARF_NAMEFY(DW_AT_ranges);
    DWARF_NAMEFY(DW_AT_trampoline);
    DWARF_NAMEFY(DW_AT_call_column);
    DWARF_NAMEFY(DW_AT_call_file);
    DWARF_NAMEFY(DW_AT_call_line);
    DWARF_NAMEFY(DW_AT_description);
    DWARF_NAMEFY(DW_AT_binary_scale);
    DWARF_NAMEFY(DW_AT_decimal_scale);
    DWARF_NAMEFY(DW_AT_small);
    DWARF_NAMEFY(DW_AT_decimal_sign);
    DWARF_NAMEFY(DW_AT_digit_count);
    DWARF_NAMEFY(DW_AT_picture_string);
    DWARF_NAMEFY(DW_AT_mutable);
    DWARF_NAMEFY(DW_AT_threads_scaled);
    DWARF_NAMEFY(DW_AT_explicit);
    DWARF_NAMEFY(DW_AT_object_pointer);
    DWARF_NAMEFY(DW_AT_endianity);
    DWARF_NAMEFY(DW_AT_elemental);
    DWARF_NAMEFY(DW_AT_pure);
    DWARF_NAMEFY(DW_AT_recursive);
    DWARF_NAMEFY(DW_AT_signature);
    DWARF_NAMEFY(DW_AT_main_subprogram);
    DWARF_NAMEFY(DW_AT_data_bit_offset);
    DWARF_NAMEFY(DW_AT_const_expr);
    DWARF_NAMEFY(DW_AT_enum_class);
    DWARF_NAMEFY(DW_AT_linkage_name);
    default:
      return "DW_AT_Unknown";
  }
}

const char*
dwarf_form_name(Dwarf_Form form) {
  switch (form) {
    DWARF_NAMEFY(DW_FORM_addr);
    DWARF_NAMEFY(DW_FORM_block2);
    DWARF_NAMEFY(DW_FORM_block4);
    DWARF_NAMEFY(DW_FORM_data2);
    DWARF_NAMEFY(DW_FORM_data4);
    DWARF_NAMEFY(DW_FORM_data8);
    DWARF_NAMEFY(DW_FORM_string);
    DWARF_NAMEFY(DW_FORM_block);
    DWARF_NAMEFY(DW_FORM_block1);
    DWARF_NAMEFY(DW_FORM_data1);
    DWARF_NAMEFY(DW_FORM_flag);
    DWARF_NAMEFY(DW_FORM_sdata);
    DWARF_NAMEFY(DW_FORM_strp);
    DWARF_NAMEFY(DW_FORM_udata);
    DWARF_NAMEFY(DW_FORM_ref_addr);
    DWARF_NAMEFY(DW_FORM_ref1);
    DWARF_NAMEFY(DW_FORM_ref2);
    DWARF_NAMEFY(DW_FORM_ref4);
    DWARF_NAMEFY(DW_FORM_ref8);
    DWARF_NAMEFY(DW_FORM_ref_udata);
    DWARF_NAMEFY(DW_FORM_indirect);
    DWARF_NAMEFY(DW_FORM_sec_offset);
    DWARF_NAMEFY(DW_FORM_exprloc);
    DWARF_NAMEFY(DW_FORM_flag_present);
    DWARF_NAMEFY(DW_FORM_ref_sig8);
    default:
      return "DW_FORM_Unknown";
  }
}

const char*
dwarf_tag_name(Dwarf_Tag tag) {
  switch (tag) {
    DWARF_NAMEFY(DW_TAG_array_type);
    DWARF_NAMEFY(DW_TAG_class_type);
    DWARF_NAMEFY(DW_TAG_entry_point);
    DWARF_NAMEFY(DW_TAG_enumeration_type);
    DWARF_NAMEFY(DW_TAG_formal_parameter);
    DWARF_NAMEFY(DW_TAG_imported_declaration);
    DWARF_NAMEFY(DW_TAG_label);
    DWARF_NAMEFY(DW_TAG_lexical_block);
    DWARF_NAMEFY(DW_TAG_member);
    DWARF_NAMEFY(DW_TAG_pointer_type);
    DWARF_NAMEFY(DW_TAG_reference_type);
    DWARF_NAMEFY(DW_TAG_compile_unit);
    DWARF_NAMEFY(DW_TAG_string_type);
    DWARF_NAMEFY(DW_TAG_structure_type);
    DWARF_NAMEFY(DW_TAG_subroutine_type);
    DWARF_NAMEFY(DW_TAG_typedef);
    DWARF_NAMEFY(DW_TAG_union_type);
    DWARF_NAMEFY(DW_TAG_unspecified_parameters);
    DWARF_NAMEFY(DW_TAG_variant);
    DWARF_NAMEFY(DW_TAG_common_block);
    DWARF_NAMEFY(DW_TAG_common_inclusion);
    DWARF_NAMEFY(DW_TAG_inheritance);
    DWARF_NAMEFY(DW_TAG_inlined_subroutine);
    DWARF_NAMEFY(DW_TAG_module);
    DWARF_NAMEFY(DW_TAG_ptr_to_member_type);
    DWARF_NAMEFY(DW_TAG_set_type);
    DWARF_NAMEFY(DW_TAG_subrange_type);
    DWARF_NAMEFY(DW_TAG_with_stmt);
    DWARF_NAMEFY(DW_TAG_access_declaration);
    DWARF_NAMEFY(DW_TAG_base_type);
    DWARF_NAMEFY(DW_TAG_catch_block);
    DWARF_NAMEFY(DW_TAG_const_type);
    DWARF_NAMEFY(DW_TAG_constant);
    DWARF_NAMEFY(DW_TAG_enumerator);
    DWARF_NAMEFY(DW_TAG_file_type);
    DWARF_NAMEFY(DW_TAG_friend);
    DWARF_NAMEFY(DW_TAG_namelist);
    DWARF_NAMEFY2(DW_TAG_namelist_item, DW_TAG_namelist_items);
    DWARF_NAMEFY(DW_TAG_packed_type);
    DWARF_NAMEFY(DW_TAG_subprogram);
    DWARF_NAMEFY2(DW_TAG_template_type_parameter, DW_TAG_template_type_param);
    DWARF_NAMEFY2(DW_TAG_template_value_parameter,
                  DW_TAG_template_value_param);
    DWARF_NAMEFY(DW_TAG_thrown_type);
    DWARF_NAMEFY(DW_TAG_try_block);
    DWARF_NAMEFY(DW_TAG_variant_part);
    DWARF_NAMEFY(DW_TAG_variable);
    DWARF_NAMEFY(DW_TAG_volatile_type);
    DWARF_NAMEFY(DW_TAG_dwarf_procedure);
    DWARF_NAMEFY(DW_TAG_restrict_type);
    DWARF_NAMEFY(DW_TAG_interface_type);
    DWARF_NAMEFY(DW_TAG_namespace);
    DWARF_NAMEFY(DW_TAG_imported_module);
    DWARF_NAMEFY(DW_TAG_unspecified_type);
    DWARF_NAMEFY(DW_TAG_partial_unit);
    DWARF_NAMEFY(DW_TAG_imported_unit);
    DWARF_NAMEFY(DW_TAG_mutable_type);
    DWARF_NAMEFY(DW_TAG_condition);
    DWARF_NAMEFY(DW_TAG_shared_type);
    DWARF_NAMEFY(DW_TAG_type_unit);
    DWARF_NAMEFY(DW_TAG_rvalue_reference_type);
    DWARF_NAMEFY(DW_TAG_template_alias);
    default:
      return "DW_TAG_Unknown";
  }
}

void
dump_attrib(Dwarf_At at, Dwarf_Form form, const Dwarf_Value* val) {
  if (form != 0) {
    printf("   +++ Attribute: %s [%s]\n",
           dwarf_at_name(at), dwarf_form_name(form));
  } else {
    printf("   +++ Attribute: %s\n", dwarf_at_name(at));
  }
  dump_value(val);
}

void
dump_value(const Dwarf_Value* attr_value) {
  printf("       Data[%03u]: (", attr_value->encoded_size);
  switch (attr_value->type) {
    case DWARF_VALUE_U8:
      printf("BYTE)   = %u (x%02X)\n", (Elf_Word)attr_value->u8,
                                       (Elf_Word)attr_value->u8);
      break;

    case DWARF_VALUE_S8:
      printf("SBYTE)  = %d (x%02X)\n", (Elf_Sword)attr_value->s8,
                                       (Elf_Sword)attr_value->s8);
      break;

    case DWARF_VALUE_U16:
      printf("WORD)   = %u (x%04X)\n", (Elf_Word)attr_value->u16,
                                       (Elf_Word)attr_value->u16);
      break;

    case DWARF_VALUE_S16:
      printf("SWORD)  = %d (x%04X)\n", (Elf_Sword)attr_value->s16,
                                       (Elf_Sword)attr_value->s16);
      break;

    case DWARF_VALUE_U32:
      printf("DWORD)  = %u (x%08X)\n", attr_value->u32,
                                       attr_value->u32);
      break;

    case DWARF_VALUE_S32:
      printf("SDWORD) = %d (x%08X)\n", attr_value->s32,
                                       attr_value->s32);
      break;

    case DWARF_VALUE_U64:
      printf("XWORD)  = %" PRIu64 " (x%" PRIX64 ")\n", attr_value->u64,
                                                       attr_value->u64);
      break;

    case DWARF_VALUE_S64:
      printf("SXWORD) = %" PRId64 " (x%" PRIX64 ")\n", attr_value->s64,
                                                       attr_value->s64);
      break;

    case DWARF_VALUE_STR:
      printf("STRING) = %s\n", attr_value->str);
      break;

    case DWARF_VALUE_PTR32:
      printf("PTR32)  = x%08X\n", attr_value->ptr32);
      break;

    case DWARF_VALUE_PTR64:
      printf("PTR64)  = x%08" PRIX64 "\n", attr_value->ptr64);
      break;

    case DWARF_VALUE_BLOCK:
      printf("BLOCK)  = [%u]:", attr_value->block.block_size);
      for (Elf_Xword i = 0; i < attr_value->block.block_size; i++) {
        Elf_Byte prnt = *((const Elf_Byte*)attr_value->block.block_ptr + i);
        printf(" x%02X", prnt);
      }
      printf("\n");
      break;

    case DWARF_VALUE_UNKNOWN:
    default:
      printf("UNKNOWN)");
      break;
  }
}
