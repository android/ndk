// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _ELF_TRAITS_H_
#define _ELF_TRAITS_H_

// NOTE: <stdint.h> is required here before <elf.h>. This is a NDK header bug.
#include <stdint.h>
#include <elf.h>
#include <sys/exec_elf.h>

// ELF is a traits structure used to provide convenient aliases for
// 32/64 bit Elf types, depending on the target CPU bitness.
#if __SIZEOF_POINTER__ == 4
struct ELF {
  typedef Elf32_Ehdr Ehdr;
  typedef Elf32_Phdr Phdr;
  typedef Elf32_Word Word;
  typedef Elf32_Addr Addr;
  typedef Elf32_Dyn Dyn;
  typedef Elf32_Sym Sym;
  typedef Elf32_Rel Rel;
  typedef Elf32_auxv_t auxv_t;
};
#elif __SIZEOF_POINTER__ == 8
struct ELF {
  typedef Elf64_Ehdr Ehdr;
  typedef Elf64_Phdr Phdr;
  typedef Elf64_Word Word;
  typedef Elf64_Addr Addr;
  typedef Elf64_Dyn Dyn;
  typedef Elf64_Sym Sym;
  typedef Elf64_Rel Rel;
  typedef Elf64_auxv_t auxv_t;
};
#else
#error "Unsupported target CPU bitness"
#endif

#ifdef __arm__
#define ELF_MACHINE EM_ARM
#elif defined(__i386__)
#define ELF_MACHINE EM_386
#elif defined(__mips__)
#define ELF_MACHINE EM_MIPS
#else
#error "Unsupported target CPU architecture"
#endif

#endif  // _ELF_TRAITS_H_
