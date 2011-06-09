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
 * Contains declarations of classes defined for a variety of DWARF objects.
 */

#ifndef ELFF_DWARF_DIE_H_
#define ELFF_DWARF_DIE_H_

#include "dwarf_defs.h"
#include "elf_alloc.h"

class ElfFile;
class DwarfCU;

/* Encapsulates an object that wraps up a DIE, cached during
 * ELF file parsing.
 */
class DIEObject : public DwarfAllocBase {
 public:
  /* Constructs DIEObject intance.
   * Param:
   *  die - DIE represented with this instance.
   *  parent_cu - Compilation unit this DIE belongs to.
   *  parent_die - Parent DIE object for this DIE. This parameter can be NULL
   *    only for compilation unit DIEs.
   */
  DIEObject(const Dwarf_DIE* die, DwarfCU* parent_cu, DIEObject* parent_die)
      : die_(die),
        parent_cu_(parent_cu),
        parent_die_(parent_die),
        last_child_(NULL),
        prev_sibling_(NULL) {
  }

  /* Destructs DIEObject intance. */
  ~DIEObject();

  /* Gets ELF file this DIE belongs to. */
  ElfFile* elf_file() const;

  /* Gets DWARF tag (DW_TAG_Xxx) for the DIE represented with this instance. */
  Dwarf_Tag get_tag() const;

  /* Gets the best name for this DIE.
   * Some DIEs (such as inline routine DIEs) may have no DW_AT_name property,
   * but may reference to another DIE that may contain DIE name. This method
   * tries its best to get DIE name by iterating through different methods of
   * naming the DIE.
   * Return:
   *  Name for this DIE, or NULL if it was not possible to find a relevant DIE
   *  with DW_AT_name property.
   */
  const char* get_name() const;

  /* Gets DIE's attribute by its ID.
   * Param:
   *  at_id - ID (DW_AT_Xxx) of the attribute to get.
   *  attr - Upon successful return contains requested attribute information.
   * Return:
   *  true on success, or false if attribute for the given ID doesn't exist
   *  in the DIE's attribute list.
   */
  bool get_attrib(Dwarf_At at, DIEAttrib* attr) const;

  /* Gets the leaf DIE object containing given address.
   * See DwarfCU::get_leaf_die_for_address() for method details.
   * See DIEObject::contains_address() for implementation details.
   */
  DIEObject* get_leaf_for_address(Elf_Xword address);

  /* Finds a DIE object for the given die in the branch starting with
   * this DIE object.
   */
  DIEObject* find_die_object(const Dwarf_DIE* die_to_find);

  /* Dumps this object to stdout.
   * Param:
   *  only_this - If true, only this object will be dumped. If this parameter
   *    is false, all the childs and siblings of this object will be dumped
   *    along with this object.
   */
  void dump(bool only_this) const;

 protected:
  /* Checks if this DIE object containing given address.
   * Template param:
   *  AddrType - Type of compilation unin address (4, or 8 bytes), defined by
   *    address_size field of the CU header. Must be Elf_Xword for 8 bytes
   *    address, or Elf_Word for 4 bytes address.
   * Param:
   *  address - Address ti check.
   * Return:
   *  True, if this DIE address ranges (including low_pc, high_pc attributes)
   *  contain given address, or false otherwise.
   */
  template <typename AddrType>
  bool contains_address(Elf_Xword address);

  /* Advances to the DIE's property list.
   * Param:
   *  at_abbr - Upon successful return contains a pointer to the beginning of
   *    DIE attribute abbreviation list. This parameter can be NULL, if the
   *    caller is not interested in attribute abbreviation list for this DIE.
   *  tag - Upon successful return contains DIE's tag. This parameter can be
   *    NULL, if the caller is not interested in the tag value for this DIE.
   * Return:
   *  Pointer to the beginning of the DIE attribute list in mapped .debug_info
   *  section on success, or NULL on failure.
   */
  const Elf_Byte* advance(const Dwarf_Abbr_AT** at_abbr, Dwarf_Tag* tag) const;

 public:
  /* Gets DIE represented with this instance. */
  const Dwarf_DIE* die() const {
    return die_;
  }

  /* Gets compilation unit this DIE belongs to. */
  DwarfCU* parent_cu() const {
    return parent_cu_;
  }

  /* Gets parent DIE object for this die. */
  DIEObject* parent_die() const {
    return parent_die_;
  }

  /* Gets last child object in the list of this DIE's childs. NOTE: for better
   * performace the list is created in reverse order (relatively to the order,
   * in which children DIEs have been discovered).
   */
  DIEObject* last_child() const {
    return last_child_;
  }

  /* Links next child to the list of this DIE childs. */
  void link_child(DIEObject* child) {
    last_child_ = child;
  }

  /* Gets previous sibling of this DIE in the parent's DIE object list. */
  DIEObject* prev_sibling() const {
    return prev_sibling_;
  }

  /* Links next sibling to the list of this DIE siblings. */
  void link_sibling(DIEObject* sibl) {
    prev_sibling_ = sibl;
  }

  /* Checks if this DIE object represents a CU DIE.
   * We relay here on the fact that only CU DIE objects have no parent
   * DIE objects.
   */
  bool is_cu_die() const {
    return parent_die_ == NULL;
  }

  /* Gets this DIE level in the branch.
   * DIE level defines DIE's distance from the CU DIE in the branch this DIE
   * belongs to. In other words, DIE level defines how many parent DIEs exist
   * between this DIE, and the CU DIE. For instance, the CU DIE has level 0,
   * a subroutine a() in this compilation unit has level 1, a soubroutine b(),
   * that has been inlined into subroutine a() will have level 2, a try/catch
   * block in the inlined subroutine b() will have level 3, and so on.
   */
  Elf_Word get_level() const {
    return parent_die_ != NULL ? parent_die_->get_level() + 1 : 0;
  }

 protected:
  /* DIE that is represented with this instance. */
  const Dwarf_DIE*  die_;

  /* Compilation unit this DIE belongs to. */
  DwarfCU*          parent_cu_;

  /* Parent DIE object for this die. */
  DIEObject*        parent_die_;

  /* Last child object in the list of this DIE's childs. NOTE: for better
   * performace the list is created in reverse order (relatively to the order,
   * in which children DIEs have been discovered).
   */
  DIEObject*        last_child_;

  /* Previous sibling of this DIE in the parent's DIE object list. */
  DIEObject*        prev_sibling_;
};

#endif  // ELFF_DWARF_DIE_H_
