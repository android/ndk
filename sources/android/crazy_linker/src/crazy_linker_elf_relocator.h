// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_ELF_RELOCATOR_H
#define CRAZY_LINKER_ELF_RELOCATOR_H

#include "crazy_linker_error.h"
#include "elf_traits.h"

namespace crazy {

// A class used to perform ELF relocations in a library.
// Usage is:
//   1/ Create new instance.
//   2/ Call Init().
//   3/ Call Apply() to apply all relocations.
//
// If any of these steps fail, the library will be in inconsistent
// state and must be unloaded/unmapped as soon as possible by the
// caller to avoid any issue.
class ElfRelocator {
 public:
  // Abstract class used to resolve symbol names into addresses.
  // Callers of ::Apply() should pass the address of a derived class
  // that properly implements the Lookup() method.
  class SymbolResolver {
   public:
    SymbolResolver() {}
    ~SymbolResolver() {}
    virtual void* Lookup(const char* symbol_name) = 0;
  };

  ElfRelocator();

  // Initialize the relocator. This will parse the library's dynamic
  // table to populate the structure.
  // |phdr| is the library's program header address.
  // |phdr_count| is the number of program header entries.
  // |load_bias| is the library load bias.
  // |dynamic| is the library's dynamic table.
  // |dyn_count| is the number of dynamic table entries.
  // On failure, return false and sets the |error| message.
  bool Init(const ELF::Phdr* phdr,
            size_t phdr_count,
            size_t load_bias,
            const ELF::Dyn* dynamic,
            size_t dyn_count,
            Error* error);

  // After initialization, apply all relocations in the library.
  // |resolver| must be an non-abstract instance of SymbolResolver
  // that will be used to resolve symbol lookups during relocations.
  // |string_table| is the library's string table.
  // |symbol_table| is the library's symbol table.
  // On failure, returns false and sets the |error| message.
  bool Apply(SymbolResolver* resolver,
             const char* string_table,
             const ELF::Sym* symbol_table,
             Error* error);

 private:
  bool ApplyRelocs(const ELF::Rel* rel, size_t rel_count, Error* error);

#ifdef __mips__
  bool RelocateMipsGot(Error* error);
#endif

  // Program header table
  const ELF::Phdr* phdr_;
  size_t phdr_count_;

  // Load bias to apply to all virtual address in the library.
  size_t load_bias_;

  // Relocations for the GOT and the PLT section that contains
  // the trampolines to call external symbols.
  ELF::Addr* plt_got_;
  ELF::Rel* plt_rel_;
  size_t plt_rel_count_;

  // Relocations for the rest of the library.
  ELF::Rel* rel_;
  size_t rel_count_;

#if defined(__mips__)
  // MIPS-specific relocation fields.
  ELF::Word mips_symtab_count_;
  ELF::Word mips_local_got_count_;
  ELF::Word mips_gotsym_;
#endif

  bool has_text_relocations_;
  bool has_symbolic_;

  // Used only during Apply().
  SymbolResolver* resolver_;
  const char* string_table_;
  const ELF::Sym* symbol_table_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_ELF_RELOCATOR_H
