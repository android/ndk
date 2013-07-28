// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_shared_library.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <elf.h>
#include <sys/exec_elf.h>

#include "crazy_linker_ashmem.h"
#include "crazy_linker_debug.h"
#include "crazy_linker_elf_loader.h"
#include "crazy_linker_elf_relocator.h"
#include "crazy_linker_library_list.h"
#include "crazy_linker_library_view.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_thread.h"
#include "crazy_linker_util.h"
#include "crazy_linker_wrappers.h"
#include "linker_phdr.h"

#ifndef DF_SYMBOLIC
#define DF_SYMBOLIC 2
#endif

#ifndef DF_TEXTREL
#define DF_TEXTREL 4
#endif

#ifndef DT_INIT_ARRAY
#define DT_INIT_ARRAY 25
#endif

#ifndef DT_INIT_ARRAYSZ
#define DT_INIT_ARRAYSZ 27
#endif

#ifndef DT_FINI_ARRAY
#define DT_FINI_ARRAY 26
#endif

#ifndef DT_FINI_ARRAYSZ
#define DT_FINI_ARRAYSZ 28
#endif

#ifndef DT_FLAGS
#define DT_FLAGS 30
#endif

#ifndef DT_PREINIT_ARRAY
#define DT_PREINIT_ARRAY 32
#endif

#ifndef DT_PREINIT_ARRAYSZ
#define DT_PREINIT_ARRAYSZ 33
#endif

namespace crazy {

namespace {

typedef SharedLibrary::linker_function_t linker_function_t;
typedef int (*JNI_OnLoadFunctionPtr)(void* vm, void* reserved);
typedef void (*JNI_OnUnloadFunctionPtr)(void* vm, void* reserved);

// Compute the ELF hash of a given symbol.
unsigned ElfHash(const char* name) {
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(name);
  unsigned h = 0;
  while (*ptr) {
    h = (h << 4) + *ptr++;
    unsigned g = h & 0xf0000000;
    h ^= g;
    h ^= g >> 24;
  }
  return h;
}

ELF::Sym* LookupSymbolForAddress(SharedLibrary* lib, void* address) {
  ELF::Addr elf_addr = reinterpret_cast<ELF::Addr>(address) - lib->base;

  for (size_t n = 0; n < lib->nchain; ++n) {
    ELF::Sym* sym = &lib->symtab[n];
    if (sym->st_shndx != SHN_UNDEF && elf_addr >= sym->st_value &&
        elf_addr < sym->st_value + sym->st_size) {
      return sym;
    }
  }
  return NULL;
}

// Call a constructor or destructor function pointer. Ignore
// NULL and -1 values intentionally. They correspond to markers
// in the tables, or deleted values.
// |func_type| corresponds to the type of the function, and is only
// used for debugging (examples are "DT_INIT", "DT_INIT_ARRAY", etc...).
void CallFunction(linker_function_t func, const char* func_type) {
  uintptr_t func_address = reinterpret_cast<uintptr_t>(func);

  LOG("%s: %p %s\n", __FUNCTION__, func, func_type);
  if (func_address != 0 && func_address != uintptr_t(-1))
    func();
}

// Parse the dynamic section of |lib| and populate various important
// fields from it. On success, returns true. On failure, returns false,
// and sets |error| message.
bool ParseLibraryDynamicTable(SharedLibrary* lib, Error* error) {
  const ELF::Phdr* phdr = lib->phdr;
  size_t phdr_count = lib->phnum;
  ELF::Addr base = lib->base;

  phdr_table_get_dynamic_section(phdr,
                                 phdr_count,
                                 base,
                                 &lib->dynamic,
                                 &lib->dynamic_count,
                                 &lib->dynamic_flags);
  if (!lib->dynamic) {
    *error = "No PT_DYNAMIC section!";
    return false;
  }

#ifdef __arm__
  (void)phdr_table_get_arm_exidx(
      phdr, phdr_count, base, &lib->ARM_exidx, &lib->ARM_exidx_count);
#endif

  for (ELF::Dyn* dyn = lib->dynamic; dyn->d_tag != DT_NULL; ++dyn) {
    ELF::Addr dyn_value = dyn->d_un.d_val;
    uintptr_t dyn_addr = base + dyn->d_un.d_ptr;
    switch (dyn->d_tag) {
      case DT_HASH:
        LOG("  DT_HASH addr=%p\n", dyn_addr);
        {
          uintptr_t* data = reinterpret_cast<uintptr_t*>(dyn_addr);
          lib->nbucket = data[0];
          lib->nchain = data[1];
          lib->bucket = data + 2;
          lib->chain = data + 2 + lib->nbucket;
        }
        break;
      case DT_STRTAB:
        LOG("  DT_STRTAB addr=%p\n", dyn_addr);
        lib->strtab = reinterpret_cast<const char*>(dyn_addr);
        break;
      case DT_SYMTAB:
        LOG("  DT_SYMTAB addr=%p\n", dyn_addr);
        lib->symtab = reinterpret_cast<ELF::Sym*>(dyn_addr);
        break;
      case DT_DEBUG:
        // TODO(digit): Move this to a different location.
        if (lib->dynamic_flags & PF_W) {
          dyn->d_un.d_val =
              reinterpret_cast<uintptr_t>(Globals::GetRDebug()->GetAddress());
        }
        break;
      case DT_INIT:
        LOG("  DT_INIT addr=%p\n", dyn_addr);
        lib->init_func = reinterpret_cast<linker_function_t>(dyn_addr);
        break;
      case DT_FINI:
        LOG("  DT_FINI addr=%p\n", dyn_addr);
        lib->fini_func = reinterpret_cast<linker_function_t>(dyn_addr);
        break;
      case DT_INIT_ARRAY:
        LOG("  DT_INIT_ARRAY addr=%p\n", dyn_addr);
        lib->init_array = reinterpret_cast<linker_function_t*>(dyn_addr);
        break;
      case DT_INIT_ARRAYSZ:
        lib->init_array_count = dyn_value / sizeof(ELF::Addr);
        LOG("  DT_INIT_ARRAYSZ value=%p count=%p\n",
            dyn_value,
            lib->init_array_count);
        break;
      case DT_FINI_ARRAY:
        LOG("  DT_FINI_ARRAY addr=%p\n", dyn_addr);
        lib->fini_array = reinterpret_cast<linker_function_t*>(dyn_addr);
        break;
      case DT_FINI_ARRAYSZ:
        lib->fini_array_count = dyn_value / sizeof(ELF::Addr);
        LOG("  DT_FINI_ARRAYSZ value=%p count=%p\n",
            dyn_value,
            lib->fini_array_count);
        break;
      case DT_PREINIT_ARRAY:
        LOG("  DT_PREINIT_ARRAY addr=%p\n", dyn_addr);
        lib->preinit_array = reinterpret_cast<linker_function_t*>(dyn_addr);
        break;
      case DT_PREINIT_ARRAYSZ:
        lib->preinit_array_count = dyn_value / sizeof(ELF::Addr);
        LOG("  DT_PREINIT_ARRAYSZ value=%p count=%p\n",
            dyn_value,
            lib->preinit_array_count);
        break;
      case DT_SYMBOLIC:
        LOG("  DT_SYMBOLIC\n");
        lib->has_DT_SYMBOLIC = true;
        break;
      case DT_FLAGS:
        if (dyn_value & DF_SYMBOLIC)
          lib->has_DT_SYMBOLIC = true;
        break;
#if defined(__mips__)
      case DT_MIPS_RLD_MAP:
        // TODO(digit): Move this to different location.
        dyn->d_un.d_ptr =
            reinterpret_cast<ELF::Addr>(Globals::GetRDebug()->GetAddress());
        break;
#endif
      default:
        ;
    }
  }

  // Perform a few sanity checks.
  if (!lib->nbucket) {
    *error = "Missing DT_HASH entry (built with --hash-style=gnu?)";
    return false;
  }
  if (!lib->strtab) {
    *error = "Missing DT_STRTAB entry";
    return false;
  }
  if (!lib->symtab) {
    *error = "Missing DT_SYMTAB entry";
    return false;
  }
  return true;
}

bool LoadLibrary(SharedLibrary* lib,
                 const char* full_path,
                 size_t load_address,
                 size_t file_offset,
                 Error* error) {
  // First, record the path.
  LOG("%s: full path '%s'\n", __FUNCTION__, full_path);

  size_t full_path_len = strlen(full_path);
  if (full_path_len >= sizeof(lib->full_path)) {
    error->Format("Path too long: %s", full_path);
    return false;
  }

  strlcpy(lib->full_path, full_path, sizeof(lib->full_path));
  lib->base_name = GetBaseNamePtr(lib->full_path);

  // Load the ELF binary in memory.
  LOG("%s: Loading ELF segments for %s\n", __FUNCTION__, lib->base_name);

  {
    ElfLoader loader;
    if (!loader.LoadAt(lib->full_path, file_offset, load_address, error)) {
      return false;
    }

    lib->base = loader.load_start();
    lib->size = loader.load_size();
    lib->load_bias = loader.load_bias();
    lib->phnum = loader.phdr_count();
    lib->phdr = loader.loaded_phdr();
  }

  // Parse the dynamic table to extract useful information.
  LOG("%s: Parsing dynamic table of %s\n", __FUNCTION__, lib->base_name);
  if (!ParseLibraryDynamicTable(lib, error))
    return false;

  LOG("%s: Load complete for %s\n", __FUNCTION__, lib->base_name);
  return true;
}

// An instance of ElfRelocator::SymbolResolver that can be used
// to resolve symbols in a shared library being loaded by
// LibraryList::LoadLibrary.
class SharedLibraryResolver : public ElfRelocator::SymbolResolver {
 public:
  SharedLibraryResolver(SharedLibrary* lib,
                        LibraryList* lib_list,
                        Vector<LibraryView*>* dependencies)
      : lib_(lib), dependencies_(dependencies) {}

  virtual void* Lookup(const char* symbol_name) {
    // TODO(digit): Add the ability to lookup inside the main executable.

    // First, look inside the current library.
    ELF::Sym* entry = lib_->LookupSymbolEntry(symbol_name);
    if (entry)
      return reinterpret_cast<void*>(lib_->load_bias + entry->st_value);

    // Special case: redirect the dynamic linker symbols to our wrappers.
    // This ensures that loaded libraries can call dlopen() / dlsym()
    // and transparently use the crazy linker to perform their duty.
    void* address = WrapLinkerSymbol(symbol_name);
    if (address)
      return address;

    // Then look inside the dependencies.
    for (size_t n = 0; n < dependencies_->GetCount(); ++n) {
      LibraryView* wrap = (*dependencies_)[n];
      // LOG("%s: Looking into dependency %p (%s)\n", __FUNCTION__, wrap,
      // wrap->GetName());
      if (wrap->IsSystem()) {
        address = ::dlsym(wrap->GetSystem(), symbol_name);
        if (address)
          return address;
      }
      if (wrap->IsCrazy()) {
        SharedLibrary* dep = wrap->GetCrazy();
        entry = dep->LookupSymbolEntry(symbol_name);
        if (entry)
          return reinterpret_cast<void*>(dep->load_bias + entry->st_value);
      }
    }

    // Nothing found here.
    return NULL;
  }

 private:
  SharedLibrary* lib_;
  Vector<LibraryView*>* dependencies_;
};

bool RelocateLibrary(SharedLibrary* lib,
                     LibraryList* lib_list,
                     Vector<LibraryView*>* dependencies,
                     Error* error) {
  // Apply relocations.
  LOG("%s: Applying relocations to %s\n", __FUNCTION__, lib->base_name);

  ElfRelocator relocator;

  if (!relocator.Init(lib->phdr,
                      lib->phnum,
                      lib->load_bias,
                      lib->dynamic,
                      lib->dynamic_count,
                      error)) {
    return false;
  }

  SharedLibraryResolver resolver(lib, lib_list, dependencies);
  if (!relocator.Apply(&resolver, lib->strtab, lib->symtab, error))
    return false;

  LOG("%s: Relocations applied for %s\n", __FUNCTION__, lib->base_name);
  return true;
}

// Helper class for a memory mapping. This is _not_ scoped.
class Mapping {
 public:
  enum Protection {
    CAN_READ = PROT_READ,
    CAN_WRITE = PROT_WRITE,
    CAN_READ_WRITE = PROT_READ | PROT_WRITE
  };
  Mapping() : map_(NULL), size_(0) {}
  ~Mapping() {}

  // Return current mapping address.
  void* Get() { return map_; }

  // Allocate a new mapping.
  // |address| is either NULL or a fixed memory address.
  // |size| is the page-aligned size, must be > 0.
  // |prot| are the desired protection bit flags.
  // |fd| is -1 (for anonymous mappings), or a valid file descriptor.
  // on failure, return false and sets |error| message.
  bool Allocate(void* address,
                size_t size,
                Protection prot,
                int fd,
                Error* error) {
    int flags = (fd >= 0) ? MAP_SHARED : MAP_ANONYMOUS;
    if (address)
      flags |= MAP_FIXED;

    size_ = size;
    map_ = ::mmap(address, size_, static_cast<int>(prot), flags, fd, 0);
    if (map_ == MAP_FAILED) {
      error->Format("Cannot map %d bytes (addr=%p, fd=%d): %s",
                    size,
                    address,
                    fd,
                    strerror(errno));
      map_ = NULL;
      return false;
    }

    return true;
  }

  // Change the protection flags of the mapping.
  bool SetProtection(Protection prot, Error* error) {
    if (map_) {
      if (::mprotect(map_, size_, static_cast<int>(prot)) < 0) {
        char* p = reinterpret_cast<char*>(map_);
        error->Format("Cannot change protection for %p-%p: %s",
                      p,
                      p + size_,
                      strerror(errno));
        return false;
      }
    }
    return true;
  }

  // Deallocate an existing mapping, if any.
  void Deallocate() {
    if (map_) {
      ::munmap(map_, size_);
      map_ = NULL;
    }
  }

 protected:
  void* map_;
  size_t size_;
};

// Helper class for a memory mapping that is automatically
// unmapped on scope exit, unless its Release() method is called.
class ScopedMapping : public Mapping {
 public:
  void* Release() {
    void* ret = map_;
    map_ = NULL;
    return ret;
  }

  ~ScopedMapping() { Deallocate(); }
};

// Copy all the pages from |addr| and |addr + size| into an ashmem
// region identified by |fd|.
bool CopyPagesToFd(void* addr, size_t size, int fd, Error* error) {

  // Create temporary mapping of the ashmem region.
  // And copy current pages there.
  ScopedMapping fd_map;

  if (!fd_map.Allocate(NULL, size, ScopedMapping::CAN_READ_WRITE, fd, error)) {
    return false;
  }

  // Copy current pages there.
  ::memcpy(fd_map.Get(), addr, size);
  return true;
}

// Swap pages between |addr| and |addr + size| with the bytes
// from the ashmem region identified by |fd|, starting from
// a given |offset|. On failure return false and set |error| message.
bool SwapPagesFromFd(void* addr,
                     size_t size,
                     int fd,
                     size_t offset,
                     Error* error) {
  // Unmap current pages.
  if (::munmap(addr, size) < 0) {
    error->Format("%s: Could not unmap %p-%p: %s",
                  __FUNCTION__,
                  addr,
                  (char*)addr + size,
                  strerror(errno));
    return false;
  }

  // Remap the fd pages at the same location now.
  void* new_map = ::mmap(addr,
                         size,
                         PROT_READ,
                         MAP_FIXED | MAP_SHARED,
                         fd,
                         static_cast<off_t>(offset));
  if (new_map == MAP_FAILED) {
    char* p = reinterpret_cast<char*>(addr);
    error->Format("%s: Could not map %p-%p: %s",
                  __FUNCTION__,
                  p,
                  p + size,
                  strerror(errno));
    return false;
  }

#ifdef __arm__
  __clear_cache(addr, (char*)addr + size);
#endif

  // Done.
  return true;
}

bool PageEquals(const char* p1, const char* p2) {
  return ::memcmp(p1, p2, PAGE_SIZE) == 0;
}

// Conditionally swap pages between |address| and |address + size| with
// the ones from the ashmem region identified by |fd|. This only swaps
// pages that have exactly the same content. On failure, return false
// and set |error| message.
bool SwapSimilarPagesFromFd(void* addr, size_t size, int fd, Error* error) {
  // Create temporary mapping of the ashmem region.
  ScopedMapping fd_map;

  LOG("%s: Entering addr=%p size=%p fd=%d\n",
      __FUNCTION__,
      addr,
      (void*)size,
      fd);

  if (!fd_map.Allocate(NULL, size, ScopedMapping::CAN_READ, fd, error))
    return false;

  LOG("%s: mapping allocate at %p\n", __FUNCTION__, fd_map.Get());

  char* cur_page = reinterpret_cast<char*>(addr);
  char* fd_page = reinterpret_cast<char*>(fd_map.Get());
  size_t p = 0;
  size_t similar_size = 0;

  do {
    // Skip over dissimilar pages.
    while (p < size && !PageEquals(cur_page + p, fd_page + p)) {
      p += PAGE_SIZE;
    }

    // Count similar pages.
    size_t p2 = p;
    while (p2 < size && PageEquals(cur_page + p2, fd_page + p2)) {
      p2 += PAGE_SIZE;
    }

    if (p2 > p) {
      // Swap pages between |pos| and |pos2|.
      LOG("%s: Swap pages at %p-%p\n",
          __FUNCTION__,
          cur_page + p,
          cur_page + p2);
      if (!SwapPagesFromFd(cur_page + p, p2 - p, fd, p, error))
        return false;

      similar_size += (p2 - p);
    }

    p = p2;
  } while (p < size);

  LOG("%s: Swapped %d pages over %d (%d %%, %d KB not shared)\n",
      __FUNCTION__,
      similar_size / PAGE_SIZE,
      size / PAGE_SIZE,
      similar_size * 100 / size,
      (size - similar_size) / 4096);

  return true;
}

}  // namespace

SharedLibrary::SharedLibrary() {
  // Ensure all fields are empty.
  memset(this, 0, sizeof(*this));
  this->relro_fd = -1;
}

SharedLibrary::~SharedLibrary() {
  // Ensure the library is unmapped on destruction.
  if (this->base)
    munmap(reinterpret_cast<void*>(this->base), this->size);
  // Ensure the relro ashmem file descriptor is closed.
  if (this->relro_fd >= 0)
    close(this->relro_fd);
}

bool SharedLibrary::Load(const char* full_path,
                         size_t load_address,
                         size_t file_offset,
                         Error* error) {
  return LoadLibrary(this, full_path, load_address, file_offset, error);
}

bool SharedLibrary::Relocate(LibraryList* lib_list,
                             Vector<LibraryView*>* dependencies,
                             Error* error) {
  return RelocateLibrary(this, lib_list, dependencies, error);
}

ELF::Sym* SharedLibrary::LookupSymbolEntry(const char* symbol_name) {
  unsigned hash = ElfHash(symbol_name);
  ELF::Sym* symbols = this->symtab;
  const char* strings = this->strtab;

  for (unsigned n = this->bucket[hash % this->nbucket]; n != 0;
       n = this->chain[n]) {
    ELF::Sym* symbol = &symbols[n];
    // Check that the symbol has the appropriate name.
    if (strcmp(strings + symbol->st_name, symbol_name))
      continue;
    // Ignore undefined symbols.
    if (symbol->st_shndx == SHN_UNDEF)
      continue;
    // Ignore anything that isn't a global or weak definition.
    switch (ELF_ST_BIND(symbol->st_info)) {
      case STB_GLOBAL:
      case STB_WEAK:
        return symbol;
      default:
        ;
    }
  }
  return NULL;
}

void* SharedLibrary::FindAddressForSymbol(const char* symbol_name) {
  ELF::Sym* entry = LookupSymbolEntry(symbol_name);
  if (!entry)
    return NULL;

  return reinterpret_cast<void*>(this->load_bias + entry->st_value);
}

ELF::Sym* SharedLibrary::FindSymbolForAddress(void* address) {
  return LookupSymbolForAddress(this, address);
}

bool SharedLibrary::EnableSharedRelro(Error* error) {
  if (this->relro_used) {
    *error = "Shared RELRO already used in library";
    return false;
  }

  if (this->relro_fd != -1) {
    // Shared RELRO is already enabled, nothing to do here.
    return true;
  }

  ELF::Addr relro_start, relro_size;
  if (phdr_table_get_relro_info(
          this->phdr, this->phnum, this->load_bias, &relro_start, &relro_size) <
      0) {
    // No RELRO section to share.
    return true;
  }

  if (relro_size == 0) {
    // Probably means there is no real RELRO section.
    return true;
  }

  // Allocate new ashmem region.
  this->relro_start = relro_start;
  this->relro_size = relro_size;
  AshmemRegion ashmem;
  String relro_name("RELRO:");
  relro_name += this->base_name;
  if (!ashmem.Allocate(relro_size, relro_name.c_str())) {
    error->Format("Could not allocate ashmem region: %s", strerror(errno));
    return false;
  }

  void* relro_addr = reinterpret_cast<void*>(relro_start);
  if (!CopyPagesToFd(relro_addr, relro_size, ashmem.Get(), error))
    return false;

  // Ensure the ashmem region content isn't writable anymore.
  if (!ashmem.SetProtectionFlags(PROT_READ)) {
    error->Format("Could not make ashmem region read-only: %s",
                  strerror(errno));
    return false;
  }

  if (!SwapPagesFromFd(relro_addr, relro_size, ashmem.Get(), 0, error))
    return false;

  this->relro_fd = ashmem.Release();
  return true;
}

bool SharedLibrary::UseSharedRelro(size_t relro_start,
                                   size_t relro_size,
                                   int relro_fd,
                                   Error* error) {
  if (relro_fd < 0 || relro_size == 0) {
    // Nothing to do here.
    return true;
  }

  LOG("%s: relro_start=%p relro_size=%p relro_fd=%d\n",
      __FUNCTION__,
      (void*)relro_start,
      (void*)relro_size,
      relro_fd);

  // Sanity check: A shared RELRO is not already used.
  if (this->relro_used) {
    *error = "Library already using shared RELRO section";
    return false;
  }

  // Sanity check: A shared RELRO is not enabled.
  if (this->relro_fd != -1) {
    *error = "Library already enabled its shared RELRO";
    return false;
  }

  // Sanity check: Mapping the ashmem region with PROT_WRITE should fail
  // with EPERM.
  void* write_map =
      ::mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, relro_fd, 0);
  if (write_map != MAP_FAILED) {
    *error = "Shared RELRO ashmem region must not allow writable mappings";
    ::munmap(write_map, PAGE_SIZE);
    return false;
  }
  if (errno != EPERM) {
    error->Format("Shared RELRO writable mapping failed with error: %s",
                  strerror(errno));
    return false;
  }

  ELF::Addr lib_relro_start, lib_relro_size;
  if (phdr_table_get_relro_info(this->phdr,
                                this->phnum,
                                this->load_bias,
                                &lib_relro_start,
                                &lib_relro_size) < 0) {
    *error = "No RELRO section in library";
    return false;
  }

  if (lib_relro_start != relro_start || lib_relro_size != relro_size) {
    error->Format("RELRO mismatch addr=%p size=%p (wanted addr=%p size=%p)",
                  lib_relro_start,
                  lib_relro_size,
                  relro_start,
                  relro_size);
    return false;
  }

  void* relro_addr = reinterpret_cast<void*>(relro_start);
  if (!SwapSimilarPagesFromFd(relro_addr, relro_size, relro_fd, error))
    return false;

  this->relro_used = true;
  return true;
}

void SharedLibrary::CallConstructors() {
  CallFunction(this->init_func, "DT_INIT");
  for (size_t n = 0; n < this->init_array_count; ++n)
    CallFunction(this->init_array[n], "DT_INIT_ARRAY");
}

void SharedLibrary::CallDestructors() {
  for (size_t n = this->fini_array_count; n > 0; --n) {
    CallFunction(this->fini_array[n - 1], "DT_FINI_ARRAY");
  }
  CallFunction(this->fini_func, "DT_FINI");
}

bool SharedLibrary::SetJavaVM(void* java_vm,
                              int minimum_jni_version,
                              Error* error) {
  if (java_vm == NULL)
    return true;

  // Lookup for JNI_OnLoad, exit if it doesn't exist.
  JNI_OnLoadFunctionPtr jni_onload = reinterpret_cast<JNI_OnLoadFunctionPtr>(
      this->FindAddressForSymbol("JNI_OnLoad"));
  if (!jni_onload)
    return true;

  int jni_version = (*jni_onload)(java_vm, NULL);
  if (jni_version < minimum_jni_version) {
    error->Format("JNI_OnLoad() in %s returned %d, expected at least %d",
                  this->full_path,
                  jni_version,
                  minimum_jni_version);
    return false;
  }

  // Save the JavaVM handle for unload time.
  this->java_vm = java_vm;
  return true;
}

void SharedLibrary::CallJniOnUnload() {
  if (!this->java_vm)
    return;

  JNI_OnUnloadFunctionPtr jni_on_unload =
      reinterpret_cast<JNI_OnUnloadFunctionPtr>(
          this->FindAddressForSymbol("JNI_OnUnload"));

  if (jni_on_unload)
    (*jni_on_unload)(this->java_vm, NULL);
}

bool SharedLibrary::DependencyIterator::GetNext() {
  dep_name_ = NULL;
  for (;;) {
    if (dynamic_->d_tag == 0)
      return false;

    if (dynamic_->d_tag != DT_NEEDED) {
      dynamic_++;
      continue;
    }

    dep_name_ = strtab_ + dynamic_->d_un.d_val;
    dynamic_++;
    return true;
  }
}

}  // namespace crazy
