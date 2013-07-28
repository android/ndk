// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_elf_relocator.h"

#include <errno.h>
#include <string.h>

#include "crazy_linker_debug.h"
#include "crazy_linker_util.h"
#include "linker_phdr.h"

// Set to 1 to print debug traces during relocations.
// Default is 0 since there are _tons_ of them.
#define DEBUG_RELOCATIONS 0

#define RLOG(...) LOG_IF(DEBUG_RELOCATIONS, __VA_ARGS__)
#define RLOG_ERRNO(...) LOG_IF_ERRNO(DEBUG_RELOCATIONS, __VA_ARGS__)

#ifndef DF_SYMBOLIC
#define DF_SYMBOLIC 2
#endif

#ifndef DF_TEXTREL
#define DF_TEXTREL 4
#endif

#ifndef DT_FLAGS
#define DT_FLAGS 30
#endif

// Processor-specific relocation types supported by the linker.
#ifdef __arm__

#define R_ARM_ABS32 2
#define R_ARM_REL32 3
#define R_ARM_GLOB_DAT 21
#define R_ARM_JUMP_SLOT 22
#define R_ARM_COPY 20
#define R_ARM_RELATIVE 23

#endif  // __arm__

#ifdef __i386__

/* i386 relocations */
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8

#endif  // __i386__

namespace crazy {

namespace {

// List of known relocation types the relocator knows about.
enum RelocationType {
  RELOCATION_TYPE_UNKNOWN = 0,
  RELOCATION_TYPE_ABSOLUTE = 1,
  RELOCATION_TYPE_RELATIVE = 2,
  RELOCATION_TYPE_PC_RELATIVE = 3,
  RELOCATION_TYPE_COPY = 4,
};

// Convert an ELF relocation type info a RelocationType value.
RelocationType GetRelocationType(unsigned r_type) {
  switch (r_type) {
#ifdef __arm__
    case R_ARM_JUMP_SLOT:
    case R_ARM_GLOB_DAT:
    case R_ARM_ABS32:
      return RELOCATION_TYPE_ABSOLUTE;

    case R_ARM_REL32:
    case R_ARM_RELATIVE:
      return RELOCATION_TYPE_RELATIVE;

    case R_ARM_COPY:
      return RELOCATION_TYPE_COPY;
#endif

#ifdef __i386__
    case R_386_JMP_SLOT:
    case R_386_GLOB_DAT:
    case R_386_32:
      return RELOCATION_TYPE_ABSOLUTE;

    case R_386_RELATIVE:
      return RELOCATION_TYPE_RELATIVE;

    case R_386_PC32:
      return RELOCATION_TYPE_PC_RELATIVE;
#endif

#ifdef __mips__
    case R_MIPS_REL32:
      return RELOCATION_TYPE_RELATIVE;
#endif

    default:
      return RELOCATION_TYPE_UNKNOWN;
  }
}

}  // namespace

ElfRelocator::ElfRelocator() {}

bool ElfRelocator::Init(const ELF::Phdr* phdr,
                        size_t phdr_count,
                        size_t load_bias,
                        const ELF::Dyn* dyn,
                        size_t count,
                        Error* error) {
  phdr_ = phdr;
  phdr_count_ = phdr_count;
  load_bias_ = load_bias;

  LOG("%s: phdr=%p phdr_count=%d load_bias=%x dyn_table=%p dyn_count=%d\n",
      __FUNCTION__,
      phdr,
      (int)phdr_count,
      (int)load_bias,
      dyn,
      (int)count);

  for (; count > 0; dyn++, count--) {
    // Be paranoid.
    if (dyn->d_tag == DT_NULL)
      break;

    ELF::Addr dyn_value = dyn->d_un.d_val;
    uintptr_t dyn_addr = load_bias_ + dyn->d_un.d_ptr;

    switch (dyn->d_tag) {
      case DT_PLTREL:
        // NOTE: Yes, there is nothing to record here, the content of
        // plt_rel_ will come from DT_JMPREL instead.
        RLOG("  DT_PLTREL");
        if (dyn_value != DT_REL) {
          *error = "Unsupported DT_RELA entry in dynamic section";
          return false;
        }
        break;
      case DT_JMPREL:
        RLOG("  DT_JMPREL addr=%p\n", dyn_addr);
        plt_rel_ = reinterpret_cast<ELF::Rel*>(dyn_addr);
        break;
      case DT_PLTRELSZ:
        plt_rel_count_ = dyn_value / sizeof(ELF::Rel);
        RLOG("  DT_PLTRELSZ size=%d count=%d\n", dyn_value, plt_rel_count_);
        break;
      case DT_REL:
        RLOG("  DT_REL addr=%p\n", dyn_addr);
        rel_ = reinterpret_cast<ELF::Rel*>(dyn_addr);
        break;
      case DT_RELSZ:
        rel_count_ = dyn_value / sizeof(ELF::Rel);
        RLOG("  DT_RELSZ size=%d count=%d\n", dyn_value, rel_count_);
        break;
      case DT_PLTGOT:
        // NOTE from original linker:
        // Save this in case we decide to do lazy binding. We don't yet.
        RLOG("  DT_PLTGOT addr=%p\n", dyn_addr);
        plt_got_ = reinterpret_cast<uintptr_t*>(dyn_addr);
        break;
      case DT_RELA:
        *error = "Unsupported DT_RELA entry in dynamic section";
        return false;
      case DT_TEXTREL:
        RLOG("  DT_TEXTREL\n");
        has_text_relocations_ = true;
        break;
      case DT_SYMBOLIC:
        RLOG("  DT_SYMBOLIC\n");
        has_symbolic_ = true;
        break;
      case DT_FLAGS:
        if (dyn_value & DF_TEXTREL)
          has_text_relocations_ = true;
        if (dyn_value & DF_SYMBOLIC)
          has_symbolic_ = true;
        RLOG(" DT_FLAGS has_text_relocations=%s has_symbolic=%s\n",
             has_text_relocations_ ? "true" : "false",
             has_symbolic_ ? "true" : "false");
        break;
#if defined(__mips__)
      case DT_MIPS_SYMTABNO:
        RLOG("  DT_MIPS_SYMTABNO value=%d\n", dyn_value);
        mips_symtab_count_ = dyn_value;
        break;

      case DT_MIPS_LOCAL_GOTNO:
        RLOG("  DT_MIPS_LOCAL_GOTNO value=%d\n", dyn_value);
        mips_local_got_count_ = dyn_value;
        break;

      case DT_MIPS_GOTSYM:
        RLOG("  DT_MIPS_GOTSYM value=%d\n", dyn_value);
        mips_gotsym_ = dyn_value;
        break;
#endif
      default:
        RLOG("  UNKNOWN tag=%d value=%08x addr=%p\n",
             dyn->d_tag,
             dyn_value,
             (void*)dyn_addr);
        ;
    }
  }
  LOG("%s: Done!\n", __FUNCTION__);
  return true;
}

bool ElfRelocator::Apply(SymbolResolver* resolver,
                         const char* string_table,
                         const ELF::Sym* symbol_table,
                         Error* error) {
  resolver_ = resolver;
  string_table_ = string_table;
  symbol_table_ = symbol_table;

  LOG("%s: string_table=%p sybol_table=%p\n",
      __FUNCTION__,
      string_table,
      symbol_table);

  if (has_text_relocations_) {
    if (phdr_table_unprotect_segments(phdr_, phdr_count_, load_bias_) < 0) {
      error->Format("Can't unprotect loadable segments: %s", strerror(errno));
      return false;
    }
  }

  if (!ApplyRelocs(plt_rel_, plt_rel_count_, error) ||
      !ApplyRelocs(rel_, rel_count_, error)) {
    return false;
  }

#ifdef __mips__
  if (!RelocateMipsGot(error))
    return false;
#endif

  if (has_text_relocations_) {
    if (phdr_table_protect_segments(phdr_, phdr_count_, load_bias_) < 0) {
      error->Format("Can't protect loadable segments: %s", strerror(errno));
      return false;
    }
  }

  // Turn on GNU RELRO protection now.
  LOG("%s: Enabling GNU RELRO protection\n", __FUNCTION__);

  if (phdr_table_protect_gnu_relro(phdr_, phdr_count_, load_bias_) < 0) {
    error->Format("Can't enable GNU RELRO protection: %s", strerror(errno));
    return false;
  }

  LOG("%s: Done\n", __FUNCTION__);
  return true;
}

bool ElfRelocator::ApplyRelocs(const ELF::Rel* rel,
                               size_t rel_count,
                               Error* error) {
  RLOG("%s: rel=%p rel_count=%d\n", __FUNCTION__, rel, rel_count);

  if (!rel)
    return true;

  for (size_t rel_n = 0; rel_n < rel_count; rel++, rel_n++) {
    unsigned rel_type = ELF_R_TYPE(rel->r_info);
    unsigned rel_symbol = ELF_R_SYM(rel->r_info);

    ELF::Addr sym_addr = 0;
    ELF::Addr reloc = static_cast<ELF::Addr>(rel->r_offset + load_bias_);
    RLOG("  %d/%d reloc=%p offset=%p type=%d symbol=%d\n",
         rel_n + 1,
         rel_count,
         reloc,
         rel->r_offset,
         rel_type,
         rel_symbol);

    if (rel_type == 0)
      continue;

    bool CRAZY_UNUSED resolved = false;

    // If this is a symbolic relocation, compute the symbol's address.
    if (__builtin_expect(rel_symbol != 0, 0)) {
      const char* sym_name = string_table_ + symbol_table_[rel_symbol].st_name;
      RLOG("    symbol name='%s'\n", sym_name);
      void* address = resolver_->Lookup(sym_name);
      if (address) {
        // The symbol was found, so compute its address.
        RLOG("%s: symbol %s resolved to %p\n", __FUNCTION__, sym_name, address);
        resolved = true;
        sym_addr = reinterpret_cast<ELF::Addr>(address);
      } else {
        // The symbol was not found. Normally this is an error except
        // if this is a weak reference.
        if (ELF_ST_BIND(symbol_table_[rel_symbol].st_info) != STB_WEAK) {
          error->Format("Could not find symbol '%s'", sym_name);
          return false;
        }

        resolved = true;
        RLOG("%s: weak reference to unresolved symbol %s\n",
             __FUNCTION__,
             sym_name);

        // IHI0044C AAELF 4.5.1.1:
        // Libraries are not searched to resolve weak references.
        // It is not an error for a weak reference to remain
        // unsatisfied.
        //
        // During linking, the value of an undefined weak reference is:
        // - Zero if the relocation type is absolute
        // - The address of the place if the relocation is pc-relative
        // - The address of nominal base address if the relocation
        //   type is base-relative.
        RelocationType r = GetRelocationType(rel_type);
        if (r == RELOCATION_TYPE_ABSOLUTE || r == RELOCATION_TYPE_RELATIVE)
          sym_addr = 0;
        else if (r == RELOCATION_TYPE_PC_RELATIVE)
          sym_addr = reloc;
        else {
          error->Format(
              "Invalid weak relocation type (%d) for unknown symbol '%s'",
              r,
              sym_name);
          return false;
        }
      }
    }

    // Apply the relocation.
    ELF::Addr* target = reinterpret_cast<ELF::Addr*>(reloc);
    switch (rel_type) {
#ifdef __arm__
      case R_ARM_JUMP_SLOT:
        RLOG("  R_ARM_JUMP_SLOT target=%p addr=%p\n", target, sym_addr);
        *target = sym_addr;
        break;

      case R_ARM_GLOB_DAT:
        RLOG("  R_ARM_GLOB_DAT target=%p addr=%p\n", target, sym_addr);
        *target = sym_addr;
        break;

      case R_ARM_ABS32:
        RLOG("  R_ARM_ABS32 target=%p (%p) addr=%p\n",
             target,
             *target,
             sym_addr);
        *target += sym_addr;
        break;

      case R_ARM_REL32:
        RLOG("  R_ARM_REL32 target=%p (%p) addr=%p offset=%p\n",
             target,
             *target,
             sym_addr,
             rel->r_offset);
        *target += sym_addr - rel->r_offset;
        break;

      case R_ARM_RELATIVE:
        RLOG("  R_ARM_RELATIVE target=%p (%p) bias=%p\n",
             target,
             *target,
             load_bias_);
        if (__builtin_expect(rel_symbol, 0)) {
          *error = "Invalid relative relocation with symbol";
          return false;
        }
        *target += load_bias_;
        break;

      case R_ARM_COPY:
        // NOTE: These relocations are forbidden in shared libraries.
        // The Android linker has special code to deal with this, which
        // is not needed here.
        RLOG("  R_ARM_COPY\n");
        *error = "Invalid R_ARM_COPY relocation in shared library";
        return false;
#endif  // __arm__

#ifdef __i386__
      case R_386_JMP_SLOT:
        *target = sym_addr;
        break;

      case R_386_GLOB_DAT:
        *target = sym_addr;
        break;

      case R_386_RELATIVE:
        if (rel_symbol) {
          *error = "Invalid relative relocation with symbol";
          return false;
        }
        *target += load_bias_;
        break;

      case R_386_32:
        *target += sym_addr;
        break;

      case R_386_PC32:
        *target += (sym_addr - reloc);
        break;
#endif  // __i386__

#ifdef __mips__
      case R_MIPS_REL32:
        if (resolved)
          *target += sym_addr;
        else
          *target += load_bias_;
        break;
#endif  // __mips__

      default:
        error->Format("Invalid relocation type (%d)", rel_type);
        return false;
    }
  }

  return true;
}

#ifdef __mips__
bool ElfRelocator::RelocateMipsGot(Error* error) {
  if (!plt_got_)
    return true;

  // Handle the local GOT entries.
  // This mimics what the system linker does.
  // Note from the system linker:
  // got[0]: lazy resolver function address.
  // got[1]: may be used for a GNU extension.
  // Set it to a recognizable address in case someone calls it
  // (should be _rtld_bind_start).
  ELF::Addr* got = plt_got_;
  got[0] = 0xdeadbeef;
  if (got[1] & 0x80000000)
    got[1] = 0xdeadbeef;

  for (ELF::Addr n = 2; n < mips_local_got_count_; ++n)
    got[n] += load_bias_;

  // Handle the global GOT entries.
  got += mips_local_got_count_;
  const ELF::Sym* sym = symbol_table_ + mips_gotsym_;
  const ELF::Sym* sym_limit = symbol_table_ + mips_symtab_count_;
  for (; sym < sym_limit; ++sym, ++got) {
    const char* sym_name = string_table_ + sym->st_name;
    void* sym_addr = resolver_->Lookup(sym_name);
    if (sym_addr) {
      *got = reinterpret_cast<ELF::Addr>(sym_addr);
      continue;
    }

    // Undefined symbols are only ok if this is a weak reference.
    if (ELF_ST_BIND(sym->st_info) != STB_WEAK) {
      error->Format("Cannot locate symbol %s", sym_name);
      return false;
    }

    // Leave undefined symbols from weak references to 0.
    *got = 0;
  }

  return true;
}
#endif  // __mips__

}  // namespace crazy
