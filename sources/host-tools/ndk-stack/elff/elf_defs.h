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
 * Contains some helpful macros, and inline routines.
 */

#ifndef ELFF_ELF_DEFS_H_
#define ELFF_ELF_DEFS_H_

#include "elff_elf.h"

//=============================================================================
// Macros.
//=============================================================================

/* Increments a pointer by n bytes.
 * Param:
 *  p - Pointer to increment.
 *  n - Number of bytes to increment the pointer with.
 */
#define INC_PTR(p, n)   (reinterpret_cast<uint8_t*>(p) + (n))

/* Increments a constant pointer by n bytes.
 * Param:
 *  p - Pointer to increment.
 *  n - Number of bytes to increment the pointer with.
 */
#define INC_CPTR(p, n)  (reinterpret_cast<const uint8_t*>(p) + (n))

/* Increments a pointer of a given type by n bytes.
 * Param:
 *  T - Pointer type
 *  p - Pointer to increment.
 *  n - Number of bytes to increment the pointer with.
 */
#define INC_PTR_T(T, p, n)                              \
    reinterpret_cast<T*>                                \
        (reinterpret_cast<uint8_t*>(p) + (n))

/* Increments a constant pointer of a given type by n bytes.
 * Param:
 *  T - Pointer type
 *  p - Pointer to increment.
 *  n - Number of bytes to increment the pointer with.
 */
#define INC_CPTR_T(T, p, n)                                 \
    reinterpret_cast<const T*>                              \
        (reinterpret_cast<const uint8_t*>(p) + (n))

/* Calculates number of entries in a static array.
 * Param:
 *  a - Array.
 * Return:
 *  Number of entries in the array.
 */
#define ELFF_ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

/* Calculates offset of a field inside a structure (or a class) of the
 * given type.
 * Param:
 *  T - Structure (or class) type.
 *  f - Name of a field (member variable) for this structure (or class).
 */
#define ELFF_FIELD_OFFSET(T, f) ((size_t)(size_t*)&(((T *)0)->f))

//=============================================================================
// Inline routines.
//=============================================================================

/* Calculates byte interval between two pointers.
 * Param:
 *  s - Starting pointer of the interval. Must be less, or equal to 'e'.
 *  e - Ending pointer of the interval. Must be greater, or equal to 's'.
 * Return:
 *  Byte interval between two pointers.
 */
static inline size_t
diff_ptr(const void* s, const void* e) {
  assert(s <= e);
  return ((size_t)(reinterpret_cast<const uint8_t*>(e) -
         reinterpret_cast<const uint8_t*>(s)));
}

/* Gets one byte from an index inside a memory block.
 * Param:
 *  ptr - Address of the beginning of the memory block.
 *  bt - Index of a byte inside the block to get.
 * Return:
 *  A byte at the given index inside the given memory block.
 */
static inline uint8_t
get_byte(const void* ptr, uint32_t bt) {
  return *(reinterpret_cast<const uint8_t*>(ptr) + bt);
}

/* Checks if given address range is fully contained within a section.
 * Param:
 *  rp - Beginning of the range to check.
 *  rsize - Size of the range to check.
 *  ss - Beginning of the section that should contain the checking range.
 *  ssize - Size of the section that should contain the checking range.
 * Return:
 *  true, if given address range is fully contained within a section, or
 *  false, if any part of the address range is not contained in the secton.
 */
static inline bool
is_in_section(const void* rp, size_t rsize, const void* ss, size_t ssize) {
  const void* rend = INC_CPTR(rp, rsize);
  /* We also make sure here that increment didn't overflow the pointer. */
  return rp >= ss && ss != NULL && (diff_ptr(ss, rend) <= ssize) && rend >= rp;
}

/* Checks if this code runs on CPU with a little-endian data format.
 * Return:
 *  true, if this code runs on CPU with a little-endian data format,
 *  or false, if this code runs on CPU with a big-endian data format.
 */
static inline bool
is_little_endian_cpu(void) {
  uint16_t tmp = 0x00FF;
  /* Lets see if byte has flipped for little-endian. */
  return get_byte(&tmp, 0) == 0xFF;
}

#endif  // ELFF_ELF_DEFS_H_
