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
 * Contains implementation of routines that encapsulte an API for parsing
 * an ELF file containing debugging information in DWARF format.
 */

#include "elff_api.h"
#include "elf_file.h"
#include "dwarf_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

ELFF_HANDLE
elff_init(const char* elf_file_path)
{
  ElfFile* elf_file = ElfFile::Create(elf_file_path);
  return reinterpret_cast<ELFF_HANDLE>(elf_file);
}

void
elff_close(ELFF_HANDLE handle)
{
  if (handle != NULL) {
    delete reinterpret_cast<ElfFile*>(handle);
  }
}

int
elff_is_exec(ELFF_HANDLE handle)
{
  assert(handle != NULL);
  if (handle == NULL) {
    _set_errno(EINVAL);
    return -1;
  }
  return reinterpret_cast<ElfFile*>(handle)->is_exec();
}

int
elff_get_pc_address_info(ELFF_HANDLE handle,
                         uint64_t address,
                         Elf_AddressInfo* address_info)
{
  assert(handle != NULL && address_info != NULL);
  if (handle == NULL || address_info == NULL) {
    _set_errno(EINVAL);
    return -1;
  }

  if (reinterpret_cast<ElfFile*>(handle)->get_pc_address_info(address,
                                                              address_info)) {
    return 0;
  } else {
    return -1;
  }
}

void
elff_free_pc_address_info(ELFF_HANDLE handle, Elf_AddressInfo* address_info)
{
  assert(handle != NULL && address_info != NULL);
  if (handle == NULL || address_info == NULL) {
    return;
  }
  reinterpret_cast<ElfFile*>(handle)->free_pc_address_info(address_info);
}

#ifdef __cplusplus
}   /* end of extern "C" */
#endif

