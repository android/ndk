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
 * Contains declaration of a class ElfMappedSection, that encapsulates
 * a section of an ELF file, mapped to memory.
 */

#ifndef ELFF_ELF_MAPPED_SECTION_H_
#define ELFF_ELF_MAPPED_SECTION_H_

#include "elf_defs.h"
#include "mapfile.h"

/* Encapsulates a section of an ELF file, mapped to memory. */
class ElfMappedSection {
 public:
  /* Constructs ElfMappedSection instance. */
  ElfMappedSection();

  /* Destructs ElfMappedSection instance. */
  ~ElfMappedSection();

  /* Maps ELF file section to memory.
   * Param:
   *  handle - Handle to an opened ELF file.
   *  offset - Offset of the beginning of the section data in ELF file.
   *    NOTE: we explicitly use 64-bit type for this parameter, since we may
   *    still allow 32-bit library to process 64 bits ELF/DWARF formats. We
   *    really only care about section size being small enough to fit in 32
   *    bits value in this case (which seems to be always true for ELF files,
   *    as section size is encoded with 32-bit value even in 64-bit ELF file).
   *  size - Section byte size in ELF file.
   * Return:
   *  true on success, or false on failure, with errno providing extended
   *  error information.
   *  NOTE: if section has already been mapped, this method immediately
   *  returns with success.
   */
  bool map(MapFile* handle, Elf_Xword offset, Elf_Word size);

  /* Checks if section has been mapped. */
  bool is_mapped() const {
    return mapped_at_ != NULL;
  }

  /* Gets address of the beginning of the mapped section. */
  const void* data() const {
    assert(is_mapped());
    return data_;
  }

  /* Gets section size. */
  Elf_Word size() const {
    assert(is_mapped());
    return size_;
  }

  /* Checks if an address range is fully contained in this section. */
  bool is_contained(const void* ptr, size_t rsize) const {
    assert(is_mapped());
    return is_mapped() && is_in_section(ptr, rsize, data(), size());
  }

 protected:
  /* Beginning of the memory mapping, containing the section.
   * NOTE: due to page alignment requirements of the mapping API, mapping
   * address may differ from the address where the actual section data
   * starts inside that mapping.
   */
  void*         mapped_at_;

  /* Address of the beginning of the mapped section. */
  const void*   data_;

  /* Section size. */
  Elf_Word      size_;
};

#endif  // ELFF_ELF_MAPPED_SECTION_H_
