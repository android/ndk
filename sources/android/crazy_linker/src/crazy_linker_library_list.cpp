// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_library_list.h"

#include <dlfcn.h>

#include "crazy_linker_debug.h"
#include "crazy_linker_library_view.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_rdebug.h"
#include "crazy_linker_shared_library.h"
#include "crazy_linker_system.h"

namespace crazy {

namespace {

// A helper struct used when looking up symbols in libraries.
struct SymbolLookupState {
  void* found_addr;
  void* weak_addr;
  int weak_count;

  SymbolLookupState() : found_addr(NULL), weak_addr(NULL), weak_count(0) {}

  // Check a symbol entry.
  bool CheckSymbol(const char* symbol, SharedLibrary* lib) {
    const ELF::Sym* entry = lib->LookupSymbolEntry(symbol);
    if (!entry)
      return false;

    void* address = reinterpret_cast<void*>(lib->load_bias() + entry->st_value);

    // If this is a strong symbol, record it and return true.
    if (ELF_ST_BIND(entry->st_info) == STB_GLOBAL) {
      found_addr = address;
      return true;
    }
    // If this is a weak symbol, record the first one and
    // increment the weak_count.
    if (++weak_count == 1)
      weak_addr = address;

    return false;
  }
};

}  // namespace

LibraryList::LibraryList() : head_(0), count_(0), has_error_(false) {
  // Nothing for now
}

LibraryList::~LibraryList() {
  // Invalidate crazy library list.
  head_ = NULL;

  // Destroy all known libraries.
  while (!known_libraries_.IsEmpty()) {
    LibraryView* wrap = known_libraries_.PopLast();
    delete wrap;
  }
}

LibraryView* LibraryList::FindLibraryByName(const char* base_name) {
  // Sanity check.
  if (!base_name || strchr(base_name, '/'))
    return NULL;

  for (size_t n = 0; n < known_libraries_.GetCount(); ++n) {
    LibraryView* wrap = known_libraries_[n];
    if (!strcmp(base_name, wrap->GetName()))
      return wrap;
  }
  return NULL;
}

void* LibraryList::FindSymbolFrom(const char* symbol_name, LibraryView* from) {
  SymbolLookupState lookup_state;

  if (!from)
    return NULL;

  // Use a work-queue and a set to ensure to perform a breadth-first
  // search.
  Vector<LibraryView*> work_queue;
  Set<LibraryView*> visited_set;

  work_queue.PushBack(from);

  while (!work_queue.IsEmpty()) {
    LibraryView* lib = work_queue.PopFirst();
    if (lib->IsCrazy()) {
      if (lookup_state.CheckSymbol(symbol_name, lib->GetCrazy()))
        return lookup_state.found_addr;
    } else if (lib->IsSystem()) {
      // TODO(digit): Support weak symbols in system libraries.
      // With the current code, all symbols in system libraries
      // are assumed to be non-weak.
      void* addr = lib->LookupSymbol(symbol_name);
      if (addr)
        return addr;
    }

    // If this is a crazy library, add non-visited dependencies
    // to the work queue.
    if (lib->IsCrazy()) {
      SharedLibrary::DependencyIterator iter(lib->GetCrazy());
      while (iter.GetNext()) {
        LibraryView* dependency = FindKnownLibrary(iter.GetName());
        if (dependency && !visited_set.Has(dependency)) {
          work_queue.PushBack(dependency);
          visited_set.Add(dependency);
        }
      }
    }
  }

  if (lookup_state.weak_count >= 1) {
    // There was at least a single weak symbol definition, so use
    // the first one found in breadth-first search order.
    return lookup_state.weak_addr;
  }

  // There was no symbol definition.
  return NULL;
}

LibraryView* LibraryList::FindLibraryForAddress(void* address) {
  // Linearly scan all libraries, looking for one that contains
  // a given address. NOTE: This doesn't check that this falls
  // inside one of the mapped library segments.
  for (size_t n = 0; n < known_libraries_.GetCount(); ++n) {
    LibraryView* wrap = known_libraries_[n];
    // TODO(digit): Search addresses inside system libraries.
    if (wrap->IsCrazy()) {
      SharedLibrary* lib = wrap->GetCrazy();
      if (lib->ContainsAddress(address))
        return wrap;
    }
  }
  return NULL;
}

#ifdef __arm__
_Unwind_Ptr LibraryList::FindArmExIdx(void* pc, int* count) {
  for (SharedLibrary* lib = head_; lib; lib = lib->list_next_) {
    if (lib->ContainsAddress(pc)) {
      *count = static_cast<int>(lib->arm_exidx_count_);
      return reinterpret_cast<_Unwind_Ptr>(lib->arm_exidx_);
    }
  }
  *count = 0;
  return NULL;
}
#else  // !__arm__
int LibraryList::IteratePhdr(PhdrIterationCallback callback, void* data) {
  int result = 0;
  for (SharedLibrary* lib = head_; lib; lib = lib->list_next_) {
    dl_phdr_info info;
    info.dlpi_addr = lib->link_map_.l_addr;
    info.dlpi_name = lib->link_map_.l_name;
    info.dlpi_phdr = lib->phdr();
    info.dlpi_phnum = lib->phdr_count();
    result = callback(&info, sizeof(info), data);
    if (result)
      break;
  }
  return result;
}
#endif  // !__arm__

void LibraryList::UnloadLibrary(LibraryView* wrap) {
  // Sanity check.
  LOG("%s: for %s (ref_count=%d)\n",
      __FUNCTION__,
      wrap->GetName(),
      wrap->ref_count());

  if (!wrap->IsSystem() && !wrap->IsCrazy())
    return;

  if (!wrap->SafeDecrementRef())
    return;

  // If this is a crazy library, perform manual cleanup first.
  if (wrap->IsCrazy()) {
    SharedLibrary* lib = wrap->GetCrazy();

    // Remove from internal list of crazy libraries.
    if (lib->list_next_)
      lib->list_next_->list_prev_ = lib->list_prev_;
    if (lib->list_prev_)
      lib->list_prev_->list_next_ = lib->list_next_;
    if (lib == head_)
      head_ = lib->list_next_;

    // Call JNI_OnUnload, if necessary, then the destructors.
    lib->CallJniOnUnload();
    lib->CallDestructors();

    // Unload the dependencies recursively.
    SharedLibrary::DependencyIterator iter(lib);
    while (iter.GetNext()) {
      LibraryView* dependency = FindKnownLibrary(iter.GetName());
      if (dependency)
        UnloadLibrary(dependency);
    }

    // Tell GDB of this removal.
    Globals::GetRDebug()->DelEntry(&lib->link_map_);
  }

  known_libraries_.Remove(wrap);

  // Delete the wrapper, which will delete the crazy library, or
  // dlclose() the system one.
  delete wrap;
}

LibraryView* LibraryList::LoadLibrary(const char* lib_name,
                                      int dlopen_mode,
                                      uintptr_t load_address,
                                      off_t file_offset,
                                      SearchPathList* search_path_list,
                                      Error* error) {

  const char* base_name = GetBaseNamePtr(lib_name);

  LOG("%s: lib_name='%s'\n", __FUNCTION__, lib_name);

  // First check whether a library with the same base name was
  // already loaded.
  LibraryView* wrap = FindKnownLibrary(lib_name);
  if (wrap) {
    if (load_address) {
      // Check that this is a crazy library and that is was loaded at
      // the correct address.
      if (!wrap->IsCrazy()) {
        error->Format("System library can't be loaded at fixed address %08x",
                      load_address);
        return NULL;
      }
      uintptr_t actual_address = wrap->GetCrazy()->load_address();
      if (actual_address != load_address) {
        error->Format("Library already loaded at @%08x, can't load it at @%08x",
                      actual_address,
                      load_address);
        return NULL;
      }
    }
    wrap->AddRef();
    return wrap;
  }

  if (IsSystemLibrary(lib_name)) {
    // This is a system library, probably because we're loading the
    // library as a dependency.
    LOG("%s: Loading system library '%s'\n", __FUNCTION__, lib_name);
    ::dlerror();
    void* system_lib = dlopen(lib_name, dlopen_mode);
    if (!system_lib) {
      error->Format("Can't load system library %s: %s", lib_name, ::dlerror());
      return NULL;
    }

    LibraryView* wrap = new LibraryView();
    wrap->SetSystem(system_lib, lib_name);
    known_libraries_.PushBack(wrap);

    LOG("%s: System library %s loaded at %p\n", __FUNCTION__, lib_name, wrap);
    LOG("  name=%s\n", wrap->GetName());
    return wrap;
  }

  ScopedPtr<SharedLibrary> lib(new SharedLibrary());

  // Find the full library path.
  String full_path;

  if (!strchr(lib_name, '/')) {
    LOG("%s: Looking through the search path list\n", __FUNCTION__);
    const char* path = search_path_list->FindFile(lib_name);
    if (!path) {
      error->Format("Can't find library file %s", lib_name);
      return NULL;
    }
    full_path = path;
  } else {
    if (lib_name[0] != '/') {
      // Need to transform this into a full path.
      full_path = GetCurrentDirectory();
      if (full_path.size() && full_path[full_path.size() - 1] != '/')
        full_path += '/';
      full_path += lib_name;
    } else {
      // Absolute path. Easy.
      full_path = lib_name;
    }
    LOG("%s: Full library path: %s\n", __FUNCTION__, full_path.c_str());
    if (!PathIsFile(full_path.c_str())) {
      error->Format("Library file doesn't exist: %s", full_path.c_str());
      return NULL;
    }
  }

  // Load the library
  if (!lib->Load(full_path.c_str(), load_address, file_offset, error))
    return NULL;

  // Load all dependendent libraries.
  LOG("%s: Loading dependencies of %s\n", __FUNCTION__, base_name);
  SharedLibrary::DependencyIterator iter(lib.Get());
  Vector<LibraryView*> dependencies;
  while (iter.GetNext()) {
    Error dep_error;
    LibraryView* dependency = LoadLibrary(iter.GetName(),
                                          dlopen_mode,
                                          0U /* load address */,
                                          0U /* file offset */,
                                          search_path_list,
                                          &dep_error);
    if (!dependency) {
      error->Format("When loading %s: %s", base_name, dep_error.c_str());
      return NULL;
    }
    dependencies.PushBack(dependency);
  }
  if (CRAZY_DEBUG) {
    LOG("%s: Dependencies loaded for %s\n", __FUNCTION__, base_name);
    for (size_t n = 0; n < dependencies.GetCount(); ++n)
      LOG("  ... %p %s\n", dependencies[n], dependencies[n]->GetName());
    LOG("    dependencies @%p\n", &dependencies);
  }

  // Relocate the library.
  LOG("%s: Relocating %s\n", __FUNCTION__, base_name);
  if (!lib->Relocate(this, &dependencies, error))
    return NULL;

  // Notify GDB of load.
  lib->link_map_.l_addr = lib->load_address();
  lib->link_map_.l_name = const_cast<char*>(lib->base_name_);
  lib->link_map_.l_ld = reinterpret_cast<uintptr_t>(lib->view_.dynamic());
  Globals::GetRDebug()->AddEntry(&lib->link_map_);

  // The library was properly loaded, add it to the list of crazy
  // libraries. IMPORTANT: Do this _before_ calling the constructors
  // because these could call dlopen().
  lib->list_next_ = head_;
  lib->list_prev_ = NULL;
  if (head_)
    head_->list_prev_ = lib.Get();
  head_ = lib.Get();

  // Then create a new LibraryView for it.
  wrap = new LibraryView();
  wrap->SetCrazy(lib.Get(), lib_name);
  known_libraries_.PushBack(wrap);

  LOG("%s: Running constructors for %s\n", __FUNCTION__, base_name);

  // Now run the constructors.
  lib->CallConstructors();

  LOG("%s: Done loading %s\n", __FUNCTION__, base_name);
  lib.Release();

  return wrap;
}

void LibraryList::AddLibrary(LibraryView* wrap) {
  known_libraries_.PushBack(wrap);
}

LibraryView* LibraryList::FindKnownLibrary(const char* name) {
  const char* base_name = GetBaseNamePtr(name);
  for (size_t n = 0; n < known_libraries_.GetCount(); ++n) {
    LibraryView* wrap = known_libraries_[n];
    if (!strcmp(base_name, wrap->GetName()))
      return wrap;
  }
  return NULL;
}

}  // namespace crazy
