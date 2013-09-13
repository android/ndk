// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_SHARED_LIBRARY_H
#define CRAZY_LINKER_SHARED_LIBRARY_H

#include <link.h>

#include "crazy_linker_error.h"
#include "crazy_linker_rdebug.h"
#include "crazy_linker_util.h"
#include "elf_traits.h"

namespace crazy {

class LibraryList;
class LibraryView;

// A class that models a shared library loaded by the crazy linker.

// Libraries have dependencies (which are listed in their dynamic section
// as DT_NEEDED entries). Circular dependencies are forbidden, so they
// form an ADG, where the root is the crazy linker itself, since all
// libraries that it loads will depend on it (to ensure their
// dlopen/dlsym/dlclose calls are properly wrapped).

class SharedLibrary {
 public:
  const ELF::Phdr* phdr;
  size_t phnum;
  ELF::Addr load_bias;

  ELF::Addr entry;
  ELF::Addr base;
  size_t size;

  ELF::Dyn* dynamic;
  size_t dynamic_count;
  ELF::Word dynamic_flags;

  SharedLibrary* list_next;
  SharedLibrary* list_prev;
  unsigned flags;

  const char* strtab;
  ELF::Sym* symtab;

  size_t nbucket;
  size_t nchain;
  unsigned* bucket;
  unsigned* chain;

  typedef void (*linker_function_t)();

  linker_function_t* preinit_array;
  size_t preinit_array_count;
  linker_function_t* init_array;
  size_t init_array_count;
  linker_function_t* fini_array;
  size_t fini_array_count;
  linker_function_t init_func;
  linker_function_t fini_func;

#ifdef __arm__
  // ARM EABI section used for stack unwinding.
  unsigned* ARM_exidx;
  size_t ARM_exidx_count;
#endif

  link_map_t link_map;

  bool has_DT_SYMBOLIC;

  size_t relro_start;
  size_t relro_size;
  int relro_fd;
  bool relro_used;

  void* java_vm;

  const char* base_name;
  char full_path[512];

  SharedLibrary();
  ~SharedLibrary();

  // Load a library (without its dependents) from an ELF file.
  // Note: This does not apply relocations, nor runs constructors.
  // |full_path| if the file full path.
  // |load_address| is the page-aligned load address in memory, or 0.
  // |file_offset| is the page-aligned file offset.
  // On failure, return false and set |error| message.
  //
  // After this, the caller should load all library dependencies,
  // Then call Relocate() and CallConstructors() to complete the
  // operation.
  bool Load(const char* full_path,
            size_t load_address,
            size_t file_offset,
            Error* error);

  // Relocate this library, assuming all its dependencies are already
  // loaded in |lib_list|. On failure, return false and set |error|
  // message.
  bool Relocate(LibraryList* lib_list,
                Vector<LibraryView*>* dependencies,
                Error* error);

  // Call all constructors in the library.
  void CallConstructors();

  // Call all destructors in the library.
  void CallDestructors();

  // Return the ELF symbol entry for a given symbol, if defined by
  // this library, or NULL otherwise.
  ELF::Sym* LookupSymbolEntry(const char* symbol_name);

  // Return the address of a given |symbol_name| if it is exported
  // by the library, NULL otherwise.
  void* FindAddressForSymbol(const char* symbol_name);

  // Find the symbol entry in this library that matches a given
  // address in memory. Returns NULL on failure.
  ELF::Sym* FindSymbolForAddress(void* address);

  // Prepare the relro section for sharing with another process.
  // On success, return true and sets relro_fd to an ashmem file
  // descriptor holding the shared RELRO section. On failure
  // return false ands sets |error| message.
  bool EnableSharedRelro(Error* error);

  // Try to use a shared relro section from another process.
  // On success, return true. On failure return false and
  // sets |error| message.
  bool UseSharedRelro(size_t relro_start,
                      size_t relro_size,
                      int relro_fd,
                      Error* error);

  // Look for a symbol named 'JNI_OnLoad' in this library, and if it
  // exists, call it with |java_vm| as the first parameter. If the
  // function result is less than |minimum_jni_version|, fail with
  // a message in |error|. On success, return true, and record
  // |java_vm| to call 'JNI_OnUnload' at unload time, if present.
  bool SetJavaVM(void* java_vm, int minimum_jni_version, Error* error);

  // Call 'JNI_OnUnload()' is necessary, i.e. if there was a succesful call
  // to SetJavaVM() before. This will pass the same |java_vm| value to the
  // callback, if it is present in the library.
  void CallJniOnUnload();

  // Helper class to iterate over dependencies in a given SharedLibrary.
  // Usage:
  //    SharedLibary::DependencyIterator iter(lib);
  //    while (iter.GetNext() {
  //      dependency_name = iter.GetName();
  //      ...
  //    }
  class DependencyIterator {
   public:
    DependencyIterator(SharedLibrary* lib)
        : dynamic_(lib->dynamic), strtab_(lib->strtab), dep_name_(NULL) {}

    bool GetNext();

    const char* GetName() const { return dep_name_; }

   private:
    DependencyIterator();
    DependencyIterator(const DependencyIterator&);
    DependencyIterator& operator=(const DependencyIterator&);

    ELF::Dyn* dynamic_;
    const char* strtab_;
    const char* dep_name_;
  };
};

}  // namespace crazy

#endif  // CRAZY_LINKER_SHARED_LIBRARY_H