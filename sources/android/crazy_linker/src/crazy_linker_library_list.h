// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_LIBRARY_LIST_H
#define CRAZY_LINKER_LIBRARY_LIST_H

#include <link.h>

#include "crazy_linker_error.h"
#include "crazy_linker_search_path_list.h"
#include "elf_traits.h"

// This header contains definitions related to the global list of
// library views maintained by the crazy linker. Each library view
// points to either a crazy library, or a system one.

namespace crazy {

class SharedLibrary;
class LibraryView;

// The list of all shared libraries loaded by the crazy linker.
// IMPORTANT: This class is not thread-safe!
class LibraryList {
 public:
  LibraryList();
  ~LibraryList();

  // Find a library in the list by its base name.
  // |base_name| must not contain a directory separator.
  LibraryView* FindLibraryByName(const char* base_name);

  // Lookup for a given |symbol_name|, starting from |from_lib|
  // then through its dependencies in breadth-first search order.
  // On failure, returns NULL.
  void* FindSymbolFrom(const char* symbol_name, LibraryView* from_lib);

  // Return the address of a visible given symbol. Used to implement
  // the dlsym() wrapper. Returns NULL on failure.
  void* FindAddressForSymbol(const char* symbol_name);

  // Find a SharedLibrary that contains a given address, or NULL if none
  // could be found. This simply scans all libraries.
  LibraryView* FindLibraryForAddress(void* address);

#ifdef __arm__
  // Find the base address of the .ARM.exidx section corresponding
  // to the address |pc|, as well as the number of 8-byte entries in
  // the table into |*count|. Used to implement the wrapper for
  // dl_unwind_find_exidx().
  _Unwind_Ptr FindArmExIdx(void* pc, int* count);
#else
  typedef int (*PhdrIterationCallback)(dl_phdr_info* info,
                                       size_t info_size,
                                       void* data);

  // Loop over all loaded libraries and call the |cb| callback
  // on each iteration. If the function returns 0, stop immediately
  // and return its value. Used to implement the wrapper for
  // dl_iterate_phdr().
  int IteratePhdr(PhdrIterationCallback callback, void* data);
#endif

  // Try to load a library, possibly at a fixed address.
  // On failure, returns NULL and sets the |error| message.
  LibraryView* LoadLibrary(const char* path,
                           int dlopen_flags,
                           uintptr_t load_address,
                           off_t file_offset,
                           SearchPathList* search_path_list,
                           Error* error);

  // Unload a given shared library. This really decrements the library's
  // internal reference count. When it reaches zero, the library's
  // destructors are run, its dependencies are unloaded, then the
  // library is removed from memory.
  void UnloadLibrary(LibraryView* lib);

  // Used internally by the wrappers only.
  void AddLibrary(LibraryView* lib);

 private:
  LibraryList(const LibraryList&);
  LibraryList& operator=(const LibraryList&);

  void ClearError();

  // The list of all known libraries.
  Vector<LibraryView*> known_libraries_;

  LibraryView* FindKnownLibrary(const char* name);

  // The list of all libraries loaded by the crazy linker.
  // This does _not_ include system libraries present in known_libraries_.
  SharedLibrary* head_;

  size_t count_;
  bool has_error_;
  char error_buffer_[512];
};

}  // namespace crazy

#endif  // CRAZY_LINKER_LIBRARY_LIST_H