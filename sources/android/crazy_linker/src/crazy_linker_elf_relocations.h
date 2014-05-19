// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_ELF_RELOCATIONS_H
#define CRAZY_LINKER_ELF_RELOCATIONS_H

#include <string.h>

#include "elf_traits.h"

namespace crazy {

class ElfSymbols;
class ElfView;
class Error;

// An ElfRelocations instance holds information about relocations in a mapped
// ELF binary.
class ElfRelocations {
 public:
  ElfRelocations() { ::memset(this, 0, sizeof(*this)); }
  ~ElfRelocations() {}

  bool Init(const ElfView* view, Error* error);

  // Abstract class used to resolve symbol names into addresses.
  // Callers of ::ApplyAll() should pass the address of a derived class
  // that properly implements the Lookup() method.
  class SymbolResolver {
   public:
    SymbolResolver() {}
    ~SymbolResolver() {}
    virtual void* Lookup(const char* symbol_name) = 0;
  };

  // Apply all relocations to the target mapped ELF binary. Must be called
  // after Init().
  // |symbols| maps to the symbol entries for the target library only.
  // |resolver| can resolve symbols out of the current library.
  // On error, return false and set |error| message.
  bool ApplyAll(const ElfSymbols* symbols,
                SymbolResolver* resolver,
                Error* error);

  // This function is used to adjust relocated addresses in a copy of an
  // existing section of an ELF binary. I.e. |src_addr|...|src_addr + size|
  // must be inside the mapped ELF binary, this function will first copy its
  // content into |dst_addr|...|dst_addr + size|, then adjust all relocated
  // addresses inside the destination section as if it was loaded/mapped
  // at |map_addr|...|map_addr + size|. Only relative relocations are processed,
  // symbolic ones are ignored.
  void CopyAndRelocate(size_t src_addr,
                       size_t dst_addr,
                       size_t map_addr,
                       size_t size);

 private:
  bool ResolveSymbol(unsigned rel_type,
                     unsigned rel_symbol,
                     const ElfSymbols* symbols,
                     SymbolResolver* resolver,
                     ELF::Addr reloc,
                     ELF::Addr* sym_addr,
                     Error* error);
  bool ApplyRelaReloc(const ELF::Rela* rela,
                      ELF::Addr sym_addr,
                      bool resolved,
                      Error* error);
  bool ApplyRelReloc(const ELF::Rel* rel,
                     ELF::Addr sym_addr,
                     bool resolved,
                     Error* error);
  bool ApplyRelaRelocs(const ELF::Rela* relocs,
                       size_t relocs_count,
                       const ElfSymbols* symbols,
                       SymbolResolver* resolver,
                       Error* error);
  bool ApplyRelRelocs(const ELF::Rel* relocs,
                      size_t relocs_count,
                      const ElfSymbols* symbols,
                      SymbolResolver* resolver,
                      Error* error);
  void AdjustRelocation(ELF::Word rel_type,
                        ELF::Addr src_reloc,
                        size_t dst_delta,
                        size_t map_delta);
  void RelocateRela(size_t src_addr,
                    size_t dst_addr,
                    size_t map_addr,
                    size_t size);
  void RelocateRel(size_t src_addr,
                   size_t dst_addr,
                   size_t map_addr,
                   size_t size);

#if defined(__mips__)
  bool RelocateMipsGot(const ElfSymbols* symbols,
                       SymbolResolver* resolver,
                       Error* error);
#endif

  const ELF::Phdr* phdr_;
  size_t phdr_count_;
  size_t load_bias_;

  ELF::Addr relocations_type_;
  ELF::Addr plt_relocations_;
  size_t plt_relocations_size_;
  ELF::Addr* plt_got_;

  ELF::Addr relocations_;
  size_t relocations_size_;

#if defined(__mips__)
  // MIPS-specific relocation fields.
  ELF::Word mips_symtab_count_;
  ELF::Word mips_local_got_count_;
  ELF::Word mips_gotsym_;
#endif

  bool has_text_relocations_;
  bool has_symbolic_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_ELF_RELOCATIONS_H
