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
 * Contains implementation of class ElfAllocator, that implements memory
 * allocations for DWARF objects.
 */

#include "elf_alloc.h"
#include "elf_file.h"

ElfAllocator::ElfAllocator()
    : current_chunk_(NULL) {
}

ElfAllocator::~ElfAllocator() {
  ElfAllocatorChunk* chunk_to_free = current_chunk_;
  while (chunk_to_free != NULL) {
    ElfAllocatorChunk* next_chunk = chunk_to_free->prev;
    free(chunk_to_free);
    chunk_to_free = next_chunk;
  }
}

void* ElfAllocator::alloc(size_t size) {
  /* Lets keep everyting properly aligned. */
  size = (size + ELFALLOC_ALIGNMENT_MASK) & ~ELFALLOC_ALIGNMENT_MASK;

  if (current_chunk_ == NULL || current_chunk_->remains < size) {
    /* Allocate new chunk. */
    ElfAllocatorChunk* new_chunk =
        reinterpret_cast<ElfAllocatorChunk*>(malloc(ELF_ALLOC_CHUNK_SIZE));
    assert(new_chunk != NULL);
    if (new_chunk == NULL) {
      _set_errno(ENOMEM);
      return NULL;
    }
    new_chunk->size = ELF_ALLOC_CHUNK_SIZE;
    new_chunk->avail = INC_PTR(new_chunk, sizeof(ElfAllocatorChunk));
    new_chunk->remains = new_chunk->size - sizeof(ElfAllocatorChunk);
    new_chunk->prev = current_chunk_;
    current_chunk_ = new_chunk;
  }

  void* ret = current_chunk_->avail;
  current_chunk_->remains -= size;
  current_chunk_->avail = INC_PTR(current_chunk_->avail, size);
  return ret;
}

void* DwarfAllocBase::operator new(size_t size, const ElfFile* elf) {
  return elf->allocator()->alloc(size);
}
