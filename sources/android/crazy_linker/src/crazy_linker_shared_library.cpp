// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_shared_library.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <elf.h>

#include "crazy_linker_ashmem.h"
#include "crazy_linker_debug.h"
#include "crazy_linker_elf_loader.h"
#include "crazy_linker_elf_relocations.h"
#include "crazy_linker_library_list.h"
#include "crazy_linker_library_view.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_memory_mapping.h"
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

// An instance of ElfRelocator::SymbolResolver that can be used
// to resolve symbols in a shared library being loaded by
// LibraryList::LoadLibrary.
class SharedLibraryResolver : public ElfRelocations::SymbolResolver {
 public:
  SharedLibraryResolver(SharedLibrary* lib,
                        LibraryList* lib_list,
                        Vector<LibraryView*>* dependencies)
      : lib_(lib), dependencies_(dependencies) {}

  virtual void* Lookup(const char* symbol_name) {
    // TODO(digit): Add the ability to lookup inside the main executable.

    // First, look inside the current library.
    const ELF::Sym* entry = lib_->LookupSymbolEntry(symbol_name);
    if (entry)
      return reinterpret_cast<void*>(lib_->load_bias() + entry->st_value);

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
#ifdef __arm__
        // Android libm.so defines isnanf as weak. This means that its
        // address cannot be found by dlsym(), which always returns NULL
        // for weak symbols. However, libm.so contains the real isnanf
        // as __isnanf. If we encounter isnanf and fail to resolve it in
        // libm.so, retry with __isnanf.
        //
        // This occurs only in clang, which lacks __builtin_isnanf. The
        // gcc compiler implements isnanf as a builtin, so the symbol
        // isnanf never need be resolved in gcc builds.
        //
        // http://code.google.com/p/chromium/issues/detail?id=376828
        if (!address &&
            !strcmp(symbol_name, "isnanf") &&
            !strcmp(wrap->GetName(), "libm.so"))
          address = ::dlsym(wrap->GetSystem(), "__isnanf");
#endif
        if (address)
          return address;
      }
      if (wrap->IsCrazy()) {
        SharedLibrary* dep = wrap->GetCrazy();
        entry = dep->LookupSymbolEntry(symbol_name);
        if (entry)
          return reinterpret_cast<void*>(dep->load_bias() + entry->st_value);
      }
    }

    // Nothing found here.
    return NULL;
  }

 private:
  SharedLibrary* lib_;
  Vector<LibraryView*>* dependencies_;
};

}  // namespace

SharedLibrary::SharedLibrary() { ::memset(this, 0, sizeof(*this)); }

SharedLibrary::~SharedLibrary() {
  // Ensure the library is unmapped on destruction.
  if (view_.load_address())
    munmap(reinterpret_cast<void*>(view_.load_address()), view_.load_size());
}

bool SharedLibrary::Load(const char* full_path,
                         size_t load_address,
                         size_t file_offset,
                         Error* error) {
  // First, record the path.
  LOG("%s: full path '%s'\n", __FUNCTION__, full_path);

  size_t full_path_len = strlen(full_path);
  if (full_path_len >= sizeof(full_path_)) {
    error->Format("Path too long: %s", full_path);
    return false;
  }

  strlcpy(full_path_, full_path, sizeof(full_path_));
  base_name_ = GetBaseNamePtr(full_path_);

  // Load the ELF binary in memory.
  LOG("%s: Loading ELF segments for %s\n", __FUNCTION__, base_name_);

  {
    ElfLoader loader;
    if (!loader.LoadAt(full_path_, file_offset, load_address, error)) {
      return false;
    }

    if (!view_.InitUnmapped(loader.load_start(),
                            loader.loaded_phdr(),
                            loader.phdr_count(),
                            error)) {
      return false;
    }

    if (!symbols_.Init(&view_)) {
      *error = "Missing or malformed symbol table";
      return false;
    }
  }

  if (phdr_table_get_relro_info(view_.phdr(),
                                view_.phdr_count(),
                                view_.load_bias(),
                                &relro_start_,
                                &relro_size_) < 0) {
    relro_start_ = 0;
    relro_size_ = 0;
  }

#ifdef __arm__
  LOG("%s: Extracting ARM.exidx table for %s\n", __FUNCTION__, base_name_);
  (void)phdr_table_get_arm_exidx(
      phdr(), phdr_count(), load_bias(), &arm_exidx_, &arm_exidx_count_);
#endif

  LOG("%s: Parsing dynamic table for %s\n", __FUNCTION__, base_name_);
  ElfView::DynamicIterator dyn(&view_);
  for (; dyn.HasNext(); dyn.GetNext()) {
    ELF::Addr dyn_value = dyn.GetValue();
    uintptr_t dyn_addr = dyn.GetAddress(load_bias());
    switch (dyn.GetTag()) {
      case DT_DEBUG:
        if (view_.dynamic_flags() & PF_W) {
          *dyn.GetValuePointer() =
              reinterpret_cast<uintptr_t>(Globals::GetRDebug()->GetAddress());
        }
        break;
      case DT_INIT:
        LOG("  DT_INIT addr=%p\n", dyn_addr);
        init_func_ = reinterpret_cast<linker_function_t>(dyn_addr);
        break;
      case DT_FINI:
        LOG("  DT_FINI addr=%p\n", dyn_addr);
        fini_func_ = reinterpret_cast<linker_function_t>(dyn_addr);
        break;
      case DT_INIT_ARRAY:
        LOG("  DT_INIT_ARRAY addr=%p\n", dyn_addr);
        init_array_ = reinterpret_cast<linker_function_t*>(dyn_addr);
        break;
      case DT_INIT_ARRAYSZ:
        init_array_count_ = dyn_value / sizeof(ELF::Addr);
        LOG("  DT_INIT_ARRAYSZ value=%p count=%p\n",
            dyn_value,
            init_array_count_);
        break;
      case DT_FINI_ARRAY:
        LOG("  DT_FINI_ARRAY addr=%p\n", dyn_addr);
        fini_array_ = reinterpret_cast<linker_function_t*>(dyn_addr);
        break;
      case DT_FINI_ARRAYSZ:
        fini_array_count_ = dyn_value / sizeof(ELF::Addr);
        LOG("  DT_FINI_ARRAYSZ value=%p count=%p\n",
            dyn_value,
            fini_array_count_);
        break;
      case DT_PREINIT_ARRAY:
        LOG("  DT_PREINIT_ARRAY addr=%p\n", dyn_addr);
        preinit_array_ = reinterpret_cast<linker_function_t*>(dyn_addr);
        break;
      case DT_PREINIT_ARRAYSZ:
        preinit_array_count_ = dyn_value / sizeof(ELF::Addr);
        LOG("  DT_PREINIT_ARRAYSZ value=%p count=%p\n",
            dyn_value,
            preinit_array_count_);
        break;
      case DT_SYMBOLIC:
        LOG("  DT_SYMBOLIC\n");
        has_DT_SYMBOLIC_ = true;
        break;
      case DT_FLAGS:
        if (dyn_value & DF_SYMBOLIC)
          has_DT_SYMBOLIC_ = true;
        break;
#if defined(__mips__)
      case DT_MIPS_RLD_MAP:
        *dyn.GetValuePointer() =
            reinterpret_cast<ELF::Addr>(Globals::GetRDebug()->GetAddress());
        break;
#endif
      default:
        ;
    }
  }

  LOG("%s: Load complete for %s\n", __FUNCTION__, base_name_);
  return true;
}

bool SharedLibrary::Relocate(LibraryList* lib_list,
                             Vector<LibraryView*>* dependencies,
                             Error* error) {
  // Apply relocations.
  LOG("%s: Applying relocations to %s\n", __FUNCTION__, base_name_);

  ElfRelocations relocations;

  if (!relocations.Init(&view_, error))
    return false;

  SharedLibraryResolver resolver(this, lib_list, dependencies);
  if (!relocations.ApplyAll(&symbols_, &resolver, error))
    return false;

  LOG("%s: Relocations applied for %s\n", __FUNCTION__, base_name_);
  return true;
}

const ELF::Sym* SharedLibrary::LookupSymbolEntry(const char* symbol_name) {
  return symbols_.LookupByName(symbol_name);
}

void* SharedLibrary::FindAddressForSymbol(const char* symbol_name) {
  return symbols_.LookupAddressByName(symbol_name, view_.load_bias());
}

bool SharedLibrary::CreateSharedRelro(size_t load_address,
                                      size_t* relro_start,
                                      size_t* relro_size,
                                      int* relro_fd,
                                      Error* error) {
  SharedRelro relro;

  if (!relro.Allocate(relro_size_, base_name_, error))
    return false;

  if (load_address != 0 && load_address != this->load_address()) {
    // Need to relocate the content of the ashmem region first to accomodate
    // for the new load address.
    if (!relro.CopyFromRelocated(
             &view_, load_address, relro_start_, relro_size_, error))
      return false;
  } else {
    // Simply copy, no relocations.
    if (!relro.CopyFrom(relro_start_, relro_size_, error))
      return false;
  }

  // Enforce read-only mode for the region's content.
  if (!relro.ForceReadOnly(error))
    return false;

  // All good.
  *relro_start = relro.start();
  *relro_size = relro.size();
  *relro_fd = relro.DetachFd();
  return true;
}

bool SharedLibrary::UseSharedRelro(size_t relro_start,
                                   size_t relro_size,
                                   int relro_fd,
                                   Error* error) {
  LOG("%s: relro_start=%p relro_size=%p relro_fd=%d\n",
      __FUNCTION__,
      (void*)relro_start,
      (void*)relro_size,
      relro_fd);

  if (relro_fd < 0 || relro_size == 0) {
    // Nothing to do here.
    return true;
  }

  // Sanity check: A shared RELRO is not already used.
  if (relro_used_) {
    *error = "Library already using shared RELRO section";
    return false;
  }

  // Sanity check: RELRO addresses must match.
  if (relro_start_ != relro_start || relro_size_ != relro_size) {
    error->Format("RELRO mismatch addr=%p size=%p (wanted addr=%p size=%p)",
                  relro_start_,
                  relro_size_,
                  relro_start,
                  relro_size);
    return false;
  }

  // Everything's good, swap pages in this process's address space.
  SharedRelro relro;
  if (!relro.InitFrom(relro_start, relro_size, relro_fd, error))
    return false;

  relro_used_ = true;
  return true;
}

void SharedLibrary::CallConstructors() {
  CallFunction(init_func_, "DT_INIT");
  for (size_t n = 0; n < init_array_count_; ++n)
    CallFunction(init_array_[n], "DT_INIT_ARRAY");
}

void SharedLibrary::CallDestructors() {
  for (size_t n = fini_array_count_; n > 0; --n) {
    CallFunction(fini_array_[n - 1], "DT_FINI_ARRAY");
  }
  CallFunction(fini_func_, "DT_FINI");
}

bool SharedLibrary::SetJavaVM(void* java_vm,
                              int minimum_jni_version,
                              Error* error) {
  if (java_vm == NULL)
    return true;

  // Lookup for JNI_OnLoad, exit if it doesn't exist.
  JNI_OnLoadFunctionPtr jni_onload = reinterpret_cast<JNI_OnLoadFunctionPtr>(
      FindAddressForSymbol("JNI_OnLoad"));
  if (!jni_onload)
    return true;

  int jni_version = (*jni_onload)(java_vm, NULL);
  if (jni_version < minimum_jni_version) {
    error->Format("JNI_OnLoad() in %s returned %d, expected at least %d",
                  full_path_,
                  jni_version,
                  minimum_jni_version);
    return false;
  }

  // Save the JavaVM handle for unload time.
  java_vm_ = java_vm;
  return true;
}

void SharedLibrary::CallJniOnUnload() {
  if (!java_vm_)
    return;

  JNI_OnUnloadFunctionPtr jni_on_unload =
      reinterpret_cast<JNI_OnUnloadFunctionPtr>(
          this->FindAddressForSymbol("JNI_OnUnload"));

  if (jni_on_unload)
    (*jni_on_unload)(java_vm_, NULL);
}

bool SharedLibrary::DependencyIterator::GetNext() {
  dep_name_ = NULL;
  for (; iter_.HasNext(); iter_.GetNext()) {
    if (iter_.GetTag() == DT_NEEDED) {
      dep_name_ = symbols_->GetStringById(iter_.GetValue());
      iter_.GetNext();
      return true;
    }
  }
  return false;
}

}  // namespace crazy
