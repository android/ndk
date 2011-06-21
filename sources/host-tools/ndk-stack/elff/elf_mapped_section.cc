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
 * Contains implementation of a class ElfMappedSection, that encapsulates
 * a section of an ELF file, mapped to memory.
 */

#include "elf_defs.h"
#include "elf_mapped_section.h"

ElfMappedSection::ElfMappedSection()
    : mapped_at_(NULL),
      data_(NULL),
      size_(0) {
}

ElfMappedSection::~ElfMappedSection() {
  if (mapped_at_ != NULL) {
    mapfile_unmap(mapped_at_, diff_ptr(mapped_at_, data_) + size_);
  }
}

bool ElfMappedSection::map(MapFile* handle,
                           Elf_Xword offset,
                           Elf_Word size) {
    void* section_ptr;
    size_t mapped_bytes;
    mapped_at_ = mapfile_map(handle, offset, size, PROT_READ,
                             &section_ptr, &mapped_bytes);
    if (mapped_at_ == NULL) {
        return false;
    }

    data_ = section_ptr;
    size_ = (Elf_Word)mapped_bytes;

    return true;
}
