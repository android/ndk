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
 * Contains declaration of types, strctures, routines, etc. that encapsulte
 * an API for parsing an ELF file containing debugging information in DWARF
 * format.
 */

#ifndef ELFF_API_H_
#define ELFF_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Defines type for a handle used in ELFF API. */
typedef void* ELFF_HANDLE;

/* Defines an entry for 'inline_stack' array in Elf_AddressInfo structure.
 * Each entry in the array represents a routine, where routine represented
 * with the previous array entry has been inlined. First element in the array
 * (at index 0) represents information for the inlined routine, referenced by
 * Elf_AddressInfo structure itself. If name for a routine was not available
 * (DW_AT_name attribute was missing), routine name is set to "<unknown>".
 * Last entry in the array has all its fields set to zero. It's sufficient
 * just to check for routine_name field of this structure to be NULL to detect
 * last entry in the array.
 */
typedef struct Elf_InlineInfo {
  /* Name of the routine where previous routine is inlined.
   * This field can never be NULL, except for the last array entry.
   */
  const char*     routine_name;

  /* Source file name where routine is inlined.
   * This field can be NULL, if it was not possible to obtain information
   * about source file location for the routine. If this field is NULL, content
   * of inlined_in_file_dir and inlined_at_line fields is undefined and should
   * be ignored. */
  const char*     inlined_in_file;

  /* Source file directory where routine is inlined.
   * If inlined_in_file field contains NULL, content of this field is undefined
   * and should be ignored. */
  const char*     inlined_in_file_dir;

  /* Source file line number where routine is inlined.
   * If inlined_in_file field contains NULL, content of this field is undefined
   * and should be ignored. */
  uint32_t        inlined_at_line;
} Elf_InlineInfo;

/* Checks if an entry is the last entry in the array.
 * Return:
 *  Boolean: 1 if this is last entry, or zero otherwise.
 */
static inline int
elfinlineinfo_is_last_entry(const Elf_InlineInfo* info) {
    return info->routine_name == 0;
}

/* PC address information descriptor.
 * This descriptor contains as much information about a PC address as it was
 * possible to collect from an ELF file. */
typedef struct Elf_AddressInfo {
  /* Name of the routine containing the address. If name of the routine
   * was not available (DW_AT_name attribute was missing) this field
   * is set to "<unknown>". */
  const char*       routine_name;

  /* Name of the source file containing the routine. If source location for the
   * routine was not available, this field is set to NULL, and content of
   * dir_name, and line_number fields of this structure is not defined. */
  const char*       file_name;

  /* Path to the source file directory. If file_name field of this structure is
   * NULL, content of this field is not defined. */
  const char*       dir_name;

  /* Line number in the source file for the address. If file_name field of this
   * structure is NULL, content of this field is not defined. */
  uint32_t          line_number;

  /* If routine that contains the given address has been inlined (or it is part
   * of even deeper inline branch) this array lists information about that
   * inline branch rooting to the first routine that has not been inlined. The
   * first element in the array references a routine, where routine containing
   * the given address has been inlined. The second entry contains information
   * about a routine referenced by the first entry (and so on). If routine,
   * containing the given address has not been inlined, this field is set to
   * NULL. The array ends with an entry containing all zeroes. */
  Elf_InlineInfo*   inline_stack;
} Elf_AddressInfo;

//=============================================================================
// API routines
//=============================================================================

/* Initializes ELFF API for the given ELF file.
 * Param:
 *  elf_file_path - Path to the ELF file to initialize API for.
 * Return:
 *  On success, this routine returns a handle that can be used in subsequent
 *  calls to this API dealing with the given ELF file. On failure this routine
 *  returns NULL, with errno providing extended error information.
 *  NOTE: handle returned from this routine must be closed using elff_close().
 */
ELFF_HANDLE elff_init(const char* elf_file_path);

/* Closes a handle obtained after successful call to elff_init routine.
 * Param:
 *  handle - A handle to close. This handle must be a handle returned from
 *  a successful call to elff_init routine.
 */
void elff_close(ELFF_HANDLE handle);

/* Checks if ELF file represents an executable file, or a shared library.
 *  handle - A handle obtained from successful call to elff_init().
 * Return:
 *  1  if ELF file represents an executable file, or
 *  0  if ELF file represents a shared library, or
 *  -1 if handle is invalid.
 */
int elff_is_exec(ELFF_HANDLE handle);

/* Gets PC address information.
 * Param:
 *  handle - A handle obtained from successful call to elff_init().
 *  address - PC address to get information for. Address must be relative to
 *    the beginning of ELF file represented by the handle parameter.
 *  address_info - Upon success contains information about routine(s) that
 *    contain the given address.
 * Return:
 *  0 if routine(s) containing the given address has been found and information
 *  has been saved into address_info, or -1 if no appropriate routine for that
 *  address has been found, or there was a memory error when collecting
 *  routine(s) information. In case of failure, errno provides extended
 *  error information.
 *  NOTE: Successful call to this routine must be complimented with a call
 *  to free_pc_address_info, so ELFF API can release resources aquired for
 *  address_info.
 */
int elff_get_pc_address_info(ELFF_HANDLE handle,
                             uint64_t address,
                             Elf_AddressInfo* address_info);

/* Frees resources acquired for address information in successful call to
 * get_pc_address_info().
 * Param:
 *  handle - A handle obtained from successful call to elff_init().
 *  address_info - Address information structure, initialized in successful
 *    call to get_pc_address_info() routine.
 */
void elff_free_pc_address_info(ELFF_HANDLE handle,
                               Elf_AddressInfo* address_info);

#ifdef __cplusplus
}   /* end of extern "C" */
#endif

#endif  // ELFF_API_H_
