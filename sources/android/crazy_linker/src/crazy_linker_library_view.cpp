// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_library_view.h"

#include <dlfcn.h>
#include "crazy_linker_debug.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_shared_library.h"

namespace crazy {

LibraryView::~LibraryView() {
  LOG("%s: Destroying %s\n", __FUNCTION__, name_.c_str());
  if (type_ == TYPE_SYSTEM) {
    ::dlclose(system_);
    system_ = NULL;
  }
  if (type_ == TYPE_CRAZY) {
    delete crazy_;
    crazy_ = NULL;
  }
  type_ = TYPE_NONE;
}

void* LibraryView::LookupSymbol(const char* symbol_name) {
  if (type_ == TYPE_SYSTEM)
    return ::dlsym(system_, symbol_name);

  if (type_ == TYPE_CRAZY) {
    LibraryList* lib_list = Globals::GetLibraries();
    return lib_list->FindSymbolFrom(symbol_name, this);
  }

  return NULL;
}

bool LibraryView::GetInfo(size_t* load_address,
                          size_t* load_size,
                          size_t* relro_start,
                          size_t* relro_size,
                          int* relro_fd,
                          Error* error) {
  if (type_ != TYPE_CRAZY) {
    *error = "No RELRO sharing with system libraries";
    return false;
  }

  *load_address = crazy_->base;
  *load_size = crazy_->size;
  *relro_start = crazy_->relro_start;
  *relro_size = crazy_->relro_size;
  *relro_fd = crazy_->relro_fd;
  return true;
}

bool LibraryView::EnableSharedRelro(Error* error) {
  if (type_ != TYPE_CRAZY) {
    *error = "No RELRO sharing with system libraries";
    return false;
  }

  return crazy_->EnableSharedRelro(error);
}

bool LibraryView::UseSharedRelro(size_t relro_start,
                                 size_t relro_size,
                                 int relro_fd,
                                 Error* error) {
  if (type_ != TYPE_CRAZY) {
    *error = "No RELRO sharing with system libraries";
    return false;
  }

  // If there is no RELRO segment, don't do anything.
  if (relro_fd < 0 || relro_size == 0)
    return true;

  return crazy_->UseSharedRelro(relro_start, relro_size, relro_fd, error);
}

}  // namespace crazy
