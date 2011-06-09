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
 * Contains declaration of class ElfAllocator, that implements memory
 * allocations for DWARF objects.
 */

#ifndef ELFF_ELF_ALLOC_H_
#define ELFF_ELF_ALLOC_H_

#include <stdint.h>
#include "elff-common.h"

class ElfFile;

/* Alignment mask for blocks, allocated with this allocator. */
#define ELFALLOC_ALIGNMENT_MASK 3

/* Chunk size. Even on relatively small ELF files, there are a lot of DWARF
 * info, which makes our parsing pretty hungry on memory. On average, memory
 * consumption on cached DWARF objects may easily reach 640K, which makes
 * choosing 32K as chunk size pretty reasonable.
 */
#define ELF_ALLOC_CHUNK_SIZE  (32 * 1024)

/* Describes a chunk of memory, allocated by ElfAllocator.
 * NOTE: this header's sizeof must be always aligned accordingly to the
 * ELFALLOC_ALIGNMENT_MASK value, so we can produce properly aligned blocks
 * without having to adjust alignment of the blocks, returned from alloc()
 * method.
 */
typedef struct ElfAllocatorChunk {
  /* Previous chunk in the chain of chunks allocated by ElfAllocator instance.
   * For better allocation performance, ElfAllocator keeps its list of
   * allocated chunks in reverse order (relatively to the chunk allocation
   * sequence). So this field in each chunk references the chunk, allocated
   * just prior this one. This field contains NULL for the first allocated
   * chunk.
   */
  ElfAllocatorChunk*  prev;

  /* Address of the next available block in this chunk. */
  void*               avail;

  /* Chunk size. */
  size_t              size;

  /* Number of bytes that remain available in this chunk. */
  size_t              remains;
} ElfAllocatorChunk;

/* Encapsulates memory allocator for DWARF-related objects.
 * Due to the implementation of ELF/DWARF framework in this library, data,
 * collected during ELF/DWARF parsing stays in memory for as long, as instance
 * of ElfFile that's being parsed is alive. To save performance on the numerous
 * memory allocations (and then, deallocations) we will use this simple memory
 * allocator that will grab memory from the heap in large chunks and then will
 * provide DWARF objects with blocks of the required size inside those chunks.
 * This will be much faster than going to the heap all the time, and since we
 * will use overwritten operators new/delete for the DWARF objects that use
 * this allocator, this is going to be pretty flexible and reliable solution
 * for DWARF object allocation implementation. See DwarfAllocBase for more
 * details.
 *
 * Instance (always one) of this class is created by ElfFile object when it is
 * initializing.
 */
class ElfAllocator {
 public:
  /* Constructs ElfAllocator instance. */
  ElfAllocator();

  /* Destructs ElfAllocator instance. */
  ~ElfAllocator();

  /* Allocates requested number of bytes for a DWARF object.
   * Param:
   *  size - Number of bytes to allocate. Value passed in this parameter
   *    will be rounded up accordingly to ELFALLOC_ALIGNMENT_MASK value,
   *    simplifying alignment adjustments for the allocated blocks.
   * Return:
   *  Address of allocated block of the requested size on success,
   *  or NULL on failure.
   */
  void* alloc(size_t size);

 protected:
  /* Current chunk to allocate memory from. NOTE: chunks are listed here
   * in reverse order (relatively to the chunk allocation sequence).
   */
  ElfAllocatorChunk*  current_chunk_;
};

/* Base class for all WDARF objects that will use ElfAllocator class for
 * instance allocations. NOTE: it's required, that all classes that use
 * ElfAllocator are derived from this one, as it provides compilation-time
 * protection from mistakenly using "traditional" operator 'new' for object
 * instantiation.
 */
class DwarfAllocBase {
 public:
  /* Constructs DwarfAllocBase instance. */
  DwarfAllocBase() {
  }

  /* Destructs DwarfAllocBase instance. */
  virtual ~DwarfAllocBase() {
  }

  /* Main operator new.
   * Implements allocation of objects of derived classes from elf's "chunked"
   * allocator, instantiated in ElfFile object (see ElfAllocator class).
   * Param:
   *  size - Number of bytes to allocate for an instance of the derived class.
   *  elf - ELF file instance that owns the allocating object.
   * Return:
   *  Pointer to the allocated memory on success, or NULL on failure.
   */
  void* operator new(size_t size, const ElfFile* elf);

  /* Overwitten operator delete.
   * Since deleting for chunk-allocated objects is a "no-op", we don't do
   * anything in this operator. We, however, are obliged to implement this
   * operator in order to compliment overwritten operator 'new'.
   */
  void operator delete(void* ptr) {
  }

  /* Overwitten operator delete.
   * Since deleting for chunk-allocated objects is a "no-op", we don't do
   * anything in this operator. We, however, are obliged to implement this
   * operator in order to compliment overwritten operator 'new'.
   */
  void operator delete[](void* ptr) {
  }

 private:
  /* Default operator new.
   * We override it making 'private' in order to cause a compiler error on
   * attempts to instantiate objects of derived classes using this version
   * of operator 'new'.
   */
  void* operator new(size_t size) throw() {
    return NULL;
  }
};

extern "C" void* elff_alloc(size_t  size);
extern "C" void  elff_free(void* ptr);

#endif  // ELFF_ELF_ALLOC_H_
