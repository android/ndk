// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_wrappers.h"

#include <dlfcn.h>
#include <link.h>

#include "crazy_linker_debug.h"
#include "crazy_linker_globals.h"
#include "crazy_linker_library_list.h"
#include "crazy_linker_library_view.h"
#include "crazy_linker_shared_library.h"
#include "crazy_linker_thread.h"
#include "crazy_linker_util.h"

#ifdef __arm__
// On ARM, this function is exported by the dynamic linker but never
// declared in any official header. It is used at runtime to
// find the base address of the .ARM.exidx section for the
// shared library containing the instruction at |pc|, as well as
// the number of 8-byte entries in that section, written into |*pcount|
extern "C" _Unwind_Ptr dl_unwind_find_exidx(_Unwind_Ptr, int*);
#else
// On other architectures, this function is exported by the dynamic linker
// but never declared in any official header. It is used at runtime to
// iterate over all loaded libraries and call the |cb|. When the function
// returns non-0, the iteration returns and the function returns its
// value.
extern "C" int dl_iterate_phdr(int (*cb)(dl_phdr_info* info,
                                         size_t size,
                                         void* data),
                               void* data);
#endif

namespace crazy {

namespace {

#ifdef __arm__
extern "C" int __cxa_atexit(void (*)(void*), void*, void*);

// On ARM, this function is defined as a weak symbol by libc.so.
// Unfortunately its address cannot be found through dlsym(), which will
// always return NULL. To work-around this, define a copy here that does
// exactly the same thing. The ARM EABI mandates the function's behaviour.
// __cxa_atexit() is implemented by the C library, but not declared by
// any official header. It's part of the low-level C++ support runtime.
int __aeabi_atexit(void* object, void (*destructor)(void*), void* dso_handle) {
  return __cxa_atexit(destructor, object, dso_handle);
}
#endif

// Used to save the system dlerror() into our thread-specific data.
void SaveSystemError() {
  ThreadData* data = GetThreadData();
  data->SetError(::dlerror());
}

char* WrapDlerror() {
  ThreadData* data = GetThreadData();
  const char* error = data->GetError();
  data->SwapErrorBuffers();
  // dlerror() returns a 'char*', but no sane client code should ever
  // try to write to this location.
  return const_cast<char*>(error);
}

void* WrapDlopen(const char* path, int mode) {
  ScopedGlobalLock lock;

  // NOTE: If |path| is NULL, the wrapper should return a handle
  // corresponding to the current executable. This can't be a crazy
  // library, so don't try to handle it with the crazy linker.
  if (path) {
    LibraryList* lib_list = Globals::GetLibraries();
    Error error;
    LibraryView* wrap = lib_list->LoadLibrary(path,
                                              mode,
                                              0U /* load_address */,
                                              0U /* file_offset */,
                                              Globals::GetSearchPaths(),
                                              &error);
    if (wrap)
      return wrap;
  }

  // Try to load the executable with the system dlopen() instead.
  ::dlerror();
  void* system_lib = ::dlopen(path, mode);
  if (system_lib == NULL) {
    SaveSystemError();
    return NULL;
  }

  LibraryView* wrap_lib = new LibraryView();
  wrap_lib->SetSystem(system_lib, path ? path : "<executable>");
  Globals::GetLibraries()->AddLibrary(wrap_lib);
  return wrap_lib;
}

void* WrapDlsym(void* lib_handle, const char* symbol_name) {
  LibraryView* wrap_lib = reinterpret_cast<LibraryView*>(lib_handle);

  if (!symbol_name) {
    SetLinkerError("dlsym: NULL symbol name");
    return NULL;
  }

  if (!lib_handle) {
    SetLinkerError("dlsym: NULL library handle");
    return NULL;
  }

  // TODO(digit): Handle RTLD_DEFAULT / RTLD_NEXT
  if (lib_handle == RTLD_DEFAULT || lib_handle == RTLD_NEXT) {
    SetLinkerError("dlsym: RTLD_DEFAULT/RTLD_NEXT are not implemented!");
    return NULL;
  }

  // NOTE: The Android dlsym() only looks inside the target library,
  // while the GNU one will perform a breadth-first search into its
  // dependency tree.

  // This implementation performs a correct breadth-first search
  // when |lib_handle| corresponds to a crazy library, except that
  // it stops at system libraries that it depends on.

  void* result = NULL;

  if (wrap_lib->IsSystem()) {
    // Note: the system dlsym() only looks into the target library,
    // while the GNU linker performs a breadth-first search.
    result = ::dlsym(wrap_lib->GetSystem(), symbol_name);
    if (!result) {
      SaveSystemError();
      LOG("dlsym:%s: could not find symbol '%s' from system library\n%s",
          wrap_lib->GetName(),
          symbol_name,
          GetThreadData()->GetError());
    }
    return result;
  }

  if (wrap_lib->IsCrazy()) {
    ScopedGlobalLock lock;
    LibraryList* lib_list = Globals::GetLibraries();
    void* addr = lib_list->FindSymbolFrom(symbol_name, wrap_lib);
    if (addr)
      return addr;

    SetLinkerError("dlsym: Could not find '%s' from library '%s'",
                   symbol_name,
                   wrap_lib->GetName());
    return NULL;
  }

  SetLinkerError("dlsym: Invalid library handle %p looking for '%s'",
                 lib_handle,
                 symbol_name);
  return NULL;
}

int WrapDladdr(void* address, Dl_info* info) {
  // First, perform search in crazy libraries.
  {
    ScopedGlobalLock lock;
    LibraryList* lib_list = Globals::GetLibraries();
    LibraryView* wrap = lib_list->FindLibraryForAddress(address);
    if (wrap && wrap->IsCrazy()) {
      size_t sym_size = 0;

      SharedLibrary* lib = wrap->GetCrazy();
      ::memset(info, 0, sizeof(*info));
      info->dli_fname = lib->base_name();
      info->dli_fbase = reinterpret_cast<void*>(lib->load_address());

      // Determine if any symbol in the library contains the specified address.
      (void)lib->FindNearestSymbolForAddress(
          address, &info->dli_sname, &info->dli_saddr, &sym_size);
      return 0;
    }
  }
  // Otherwise, use system version.
  ::dlerror();
  int ret = ::dladdr(address, info);
  if (ret != 0)
    SaveSystemError();
  return ret;
}

int WrapDlclose(void* lib_handle) {
  LibraryView* wrap_lib = reinterpret_cast<LibraryView*>(lib_handle);
  if (!wrap_lib) {
    SetLinkerError("NULL library handle");
    return -1;
  }

  if (wrap_lib->IsSystem() || wrap_lib->IsCrazy()) {
    ScopedGlobalLock lock;
    LibraryList* lib_list = Globals::GetLibraries();
    lib_list->UnloadLibrary(wrap_lib);
    return 0;
  }

  // Invalid library handle!!
  SetLinkerError("Invalid library handle %p", lib_handle);
  return -1;
}

#ifdef __arm__
_Unwind_Ptr WrapDl_unwind_find_exidx(_Unwind_Ptr pc, int* pcount) {
  // First lookup in crazy libraries.
  {
    ScopedGlobalLock lock;
    LibraryList* list = Globals::GetLibraries();
    _Unwind_Ptr result = list->FindArmExIdx(pc, pcount);
    if (result)
      return result;
  }
  // Lookup in system libraries.
  return ::dl_unwind_find_exidx(pc, pcount);
}
#else  // !__arm__
int WrapDl_iterate_phdr(int (*cb)(dl_phdr_info*, size_t, void*), void* data) {
  // First, iterate over crazy libraries.
  {
    ScopedGlobalLock lock;
    LibraryList* list = Globals::GetLibraries();
    int result = list->IteratePhdr(cb, data);
    if (result)
      return result;
  }
  // Then lookup through system ones.
  return ::dl_iterate_phdr(cb, data);
}
#endif  // !__arm__

}  // namespace

void* WrapLinkerSymbol(const char* name) {
  // Shortcut, since all names begin with 'dl'
  // Take care of __aeabi_atexit on ARM though.
  if (name[0] != 'd' || name[1] != 'l') {
#ifdef __arm__
    if (name[0] == '_' && !strcmp("__aeabi_atexit", name))
      return reinterpret_cast<void*>(&__aeabi_atexit);
#endif
    return NULL;
  }

  static const struct {
    const char* name;
    void* address;
  } kSymbols[] = {
        {"dlopen", reinterpret_cast<void*>(&WrapDlopen)},
        {"dlclose", reinterpret_cast<void*>(&WrapDlclose)},
        {"dlerror", reinterpret_cast<void*>(&WrapDlerror)},
        {"dlsym", reinterpret_cast<void*>(&WrapDlsym)},
        {"dladdr", reinterpret_cast<void*>(&WrapDladdr)},
#ifdef __arm__
        {"dl_unwind_find_exidx",
         reinterpret_cast<void*>(&WrapDl_unwind_find_exidx)},
#else
        {"dl_iterate_phdr", reinterpret_cast<void*>(&WrapDl_iterate_phdr)},
#endif
    };
  static const size_t kCount = sizeof(kSymbols) / sizeof(kSymbols[0]);
  for (size_t n = 0; n < kCount; ++n) {
    if (!strcmp(kSymbols[n].name, name))
      return kSymbols[n].address;
  }
  return NULL;
}

}  // namespace crazy
