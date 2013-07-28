// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_LIBRARY_VIEW_H
#define CRAZY_LINKER_LIBRARY_VIEW_H

#include "crazy_linker_error.h"
#include "crazy_linker_util.h"

namespace crazy {

class SharedLibrary;

// A LibraryView is a reference-counted handle to either a
// crazy::SharedLibrary object or a library handle returned by the system's
// dlopen() function.
//
// It has a name, which is always a base name, because only one
// library with a given base name can be loaded in the system.
class LibraryView {
 public:
  enum {
    TYPE_NONE = 0xbaadbaad,
    TYPE_SYSTEM = 0x2387cef,
    TYPE_CRAZY = 0xcdef2387,
  };

  LibraryView()
      : type_(TYPE_NONE), crazy_(NULL), system_(NULL), name_(), ref_count_(1) {}

  ~LibraryView();

  bool IsSystem() const { return type_ == TYPE_SYSTEM; }

  bool IsCrazy() const { return type_ == TYPE_CRAZY; }

  void SetSystem(void* system_lib, const char* name) {
    type_ = TYPE_SYSTEM;
    system_ = system_lib;
    name_ = name;
  }

  void SetCrazy(SharedLibrary* crazy_lib, const char* name) {
    type_ = TYPE_CRAZY;
    crazy_ = crazy_lib;
    name_ = name;
  }

  const char* GetName() { return name_.c_str(); }

  SharedLibrary* GetCrazy() { return IsCrazy() ? crazy_ : NULL; }

  void* GetSystem() { return IsSystem() ? system_ : NULL; }

  void AddRef() { ref_count_++; }

  // Decrement reference count. Returns true iff it reaches 0.
  // This never destroys the object.
  bool SafeDecrementRef() { return (--ref_count_ == 0); }

  // Lookup a symbol from this library.
  // If this is a crazy library, perform a breadth-first search,
  // for system libraries, use dlsym() instead.
  void* LookupSymbol(const char* symbol_name);

  // Retrieve library information.
  bool GetInfo(size_t* load_address,
               size_t* load_size,
               size_t* relro_start,
               size_t* relro_size,
               int* relro_fd,
               Error* error);

  // Enable RELRO section sharing. On This moves the RELRO section into
  // a shareable ashmem region. On success, return true and sets
  // |*load_address| to the library load address, |*load_size| to its
  // full load size, |*relro_start| to the start of the RELRO section
  // in memory, |*relro_size| to its size, and |*relro_fd| to the ashmem
  // region's file descriptor.
  // On failure, return false and sets |error| message.
  bool EnableSharedRelro(Error* error);

  // Use a shared RELRO section from another process. |relro_start|
  // and |relro_size| are the start and size of the RELRO section,
  // and |relro_fd| is a file descriptor for the ashmem region.
  // On success, return true. On failure, return false and sets
  // |error| message.
  // NOTE: This function doesn't do anything except return true
  // if |relro_fd| is negative, or |relro_size| is 0. This shall correspond
  // to the case where there is no RELRO section in a library.
  bool UseSharedRelro(size_t relro_start,
                      size_t relro_size,
                      int relro_fd,
                      Error* error);

  // Only used for debugging.
  int ref_count() const { return ref_count_; }

 private:
  uint32_t type_;
  SharedLibrary* crazy_;
  void* system_;
  String name_;
  int ref_count_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_LIBRARY_VIEW_H