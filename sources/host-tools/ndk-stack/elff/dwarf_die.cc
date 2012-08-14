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
 * Contains implementations of classes defined for a variety of DWARF objects.
 */

#include <stdio.h>
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include "dwarf_die.h"
#include "dwarf_cu.h"
#include "dwarf_utils.h"
#include "elf_file.h"

DIEObject::~DIEObject() {
  /* Delete all children of this object. */
  DIEObject* to_del = last_child();
  while (to_del != NULL) {
    DIEObject* next = to_del->prev_sibling();
    delete to_del;
    to_del = next;
  }
}

ElfFile* DIEObject::elf_file() const {
  return parent_cu()->elf_file();
}

Dwarf_Tag DIEObject::get_tag() const {
  Dwarf_Tag tag;
  return advance(NULL, &tag) != NULL ? tag : 0;
}

const char* DIEObject::get_name() const {
  DIEAttrib die_attr;
  /* Start with the obvious. */
  if (get_attrib(DW_AT_name, &die_attr)) {
    return die_attr.value()->str;
  }

  /* Lets see if there is a reference to the abstract origin, or specification,
   * and use its name as the name for this DIE. */
  if (get_attrib(DW_AT_abstract_origin, &die_attr) ||
      get_attrib(DW_AT_specification, &die_attr)) {
    DIEObject* org_die_obj =
        parent_cu()->get_referenced_die_object(die_attr.value()->u32);
    if (org_die_obj != NULL) {
      return org_die_obj->get_name();
    }
  }

  /* Lets see if there is a reference to the type DIE, and use
   * its name as the name for this DIE. */
  if (get_attrib(DW_AT_type, &die_attr)) {
    DIEObject* org_die_obj =
        parent_cu()->get_referenced_die_object(die_attr.value()->u32);
    if (org_die_obj != NULL) {
      return org_die_obj->get_name();
    }
  }

  /* Can't figure the name for this DIE. */
  return NULL;
}

bool DIEObject::get_attrib(Dwarf_At at_id, DIEAttrib* attr) const {
  const Dwarf_Abbr_AT* at_abbr;

  /* Advance to DIE attributes. */
  const Elf_Byte* die_attr = advance(&at_abbr, NULL);
  if (die_attr == NULL) {
    _set_errno(EINVAL);
    return false;
  }

  /* Loop through all DIE attributes, looking for the one that's being
   * requested. */
  while (!at_abbr->is_separator()) {
    at_abbr = at_abbr->process(&attr->at_, &attr->form_);
    die_attr = parent_cu()->process_attrib(die_attr, attr->form_, &attr->value_);
    if (at_id == attr->at()) {
      return true;
    }
  }

  _set_errno(EINVAL);

  return false;
}

DIEObject* DIEObject::get_leaf_for_address(Elf_Xword address) {
  const bool contains = parent_cu()->is_CU_address_64() ?
                            contains_address<Elf_Xword>(address) :
                            contains_address<Elf_Word>(address);
  if (!contains && !is_cu_die()) {
    /* For CU DIEs address range may be zero size, even though its child DIEs
     * occupie some address space. So, if CU DIE's address range doesn't
     * contain the given address, we still want to go and check the children.
     */
    _set_errno(EINVAL);
    return NULL;
  }

  /* This DIE contains given address (or may contain it, if this is a CU DIE).
   * Lets iterate through child DIEs to find the leaf (last DIE) that contains
   * this address. */
  DIEObject* child = last_child();
  while (child != NULL) {
    DIEObject* leaf = child->get_leaf_for_address(address);
    if (leaf != NULL) {
      return leaf;
    }
    child = child->prev_sibling();
  }
  /* No child DIE contains this address. This DIE is the leaf. */
  return contains || !is_cu_die() ? this : NULL;
}

template <typename AddrType>
bool DIEObject::contains_address(Elf_Xword address) {
  DIEAttrib die_ranges;
  /* DIE can contain either list of ranges (f.i. DIEs that represent a routine
   * that is inlined in multiple places will contain list of address ranges
   * where that routine is inlined), or a pair "low PC, and high PC" describing
   * contiguos address space where routine has been placed by compiler. */
  if (get_attrib(DW_AT_ranges, &die_ranges)) {
    /* Iterate through this DIE's ranges list, looking for the one that
     * contains the given address. */
    AddrType low;
    AddrType high;
    Elf_Word range_off = die_ranges.value()->u32;
    while (elf_file()->get_range(range_off, &low, &high) &&
           (low != 0 || high != 0)) {
      if (address >= low && address < high) {
        return true;
      }
      range_off += sizeof(AddrType) * 2;
    }
    return false;
  } else {
    /* This DIE doesn't have ranges. Lets see if it has low_pc and high_pc
     * attributes. */
    DIEAttrib low_pc;
    DIEAttrib high_pc;
    if (!get_attrib(DW_AT_low_pc, &low_pc) ||
        !get_attrib(DW_AT_high_pc, &high_pc) ||
        address < low_pc.value()->u64 ||
        address >= high_pc.value()->u64) {
      return false;
    }
    return true;
  }
}

DIEObject* DIEObject::find_die_object(const Dwarf_DIE* die_to_find) {
  if (die_to_find == die()) {
    return this;
  }

  /* First we will iterate through the list of children, since chances to
   * find requested DIE decrease as we go deeper into DIE tree. */
  DIEObject* iter = last_child();
  while (iter != NULL) {
    if (iter->die() == die_to_find) {
      return iter;
    }
    iter = iter->prev_sibling();
  };

  /* DIE has not been found among the children. Lets go deeper now. */
  iter = last_child();
  while (iter != NULL) {
    DIEObject* ret = iter->find_die_object(die_to_find);
    if (ret != NULL) {
      return ret;
    }
    iter = iter->prev_sibling();
  }

  _set_errno(EINVAL);
  return NULL;
}

void DIEObject::dump(bool only_this) const {
  const Dwarf_Abbr_AT*  at_abbr;
  Dwarf_Tag             tag;

  const Elf_Byte* die_attr = advance(&at_abbr, &tag);
  if (die_attr != NULL) {
    printf("\n********** DIE[%p(%04X)] %s: %s **********\n",
           die_, parent_cu()->get_die_reference(die_), dwarf_tag_name(tag),
           get_name());

    /* Dump this DIE attributes. */
    while (!at_abbr->is_separator()) {
      DIEAttrib attr;
      at_abbr = at_abbr->process(&attr.at_, &attr.form_);
      die_attr = parent_cu()->process_attrib(die_attr, attr.form(), &attr.value_);
      dump_attrib(attr.at(), attr.form(), attr.value());
      if (attr.at() == DW_AT_ranges) {
        /* Dump all ranges for this DIE. */
        Elf_Word off = attr.value()->u32;
        if (parent_cu()->is_CU_address_64()) {
          Elf_Xword low, high;
          while (elf_file()->get_range<Elf_Xword>(off, &low, &high) &&
                 (low != 0 || high != 0)) {
            printf("                                %08" PRIX64 " - %08" PRIX64 "\n",
                   low, high);
            off += 16;
          }
        } else {
          Elf_Word low, high;
          while (elf_file()->get_range<Elf_Word>(off, &low, &high) &&
                 (low != 0 || high != 0)) {
            printf("                                %08X - %08X\n",
                   low, high);
            off += 8;
          }
        }
      }
    }
  }

  if (only_this) {
    if (parent_die_ != NULL && !parent_die_->is_cu_die()) {
      printf("\n-----------> CHILD OF:\n");
      parent_die_->dump(true);
    }
  } else {
    /* Dump this DIE's children. */
    if (last_child() != NULL) {
        last_child()->dump(false);
    }

    /* Dump this DIE's siblings. */
    if (prev_sibling() != NULL) {
      prev_sibling()->dump(false);
    }
  }
}

const Elf_Byte* DIEObject::advance(const Dwarf_Abbr_AT** at_abbr,
                                   Dwarf_Tag* tag) const {
  Dwarf_AbbrNum abbr_num;
  Dwarf_Tag     die_tag;

  const Elf_Byte* die_attr = die()->process(&abbr_num);
  const Dwarf_Abbr_DIE* abbr = parent_cu()->get_die_abbr(abbr_num);
  if (abbr == NULL) {
    return NULL;
  }

  const Dwarf_Abbr_AT* attrib_abbr = abbr->process(NULL, &die_tag);
  if (at_abbr != NULL) {
    *at_abbr = attrib_abbr;
  }
  if (tag != NULL) {
    *tag = die_tag;
  }
  return die_attr;
}
