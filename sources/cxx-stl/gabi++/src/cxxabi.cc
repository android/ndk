// Copyright (C) 2012 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <pthread.h>

#include "cxxabi_defines.h"
#include "helper_func_internal.h"

namespace {

  using namespace __cxxabiv1;

  void defaultExceptionCleanupFunc(_Unwind_Reason_Code reason,
                                   _Unwind_Exception* exc) {
    __cxa_free_exception(exc + 1);
  }

  // Technical note:
  // Use a pthread_key_t to hold the key used to store our thread-specific
  // __cxa_eh_globals objects. The key is created and destroyed through
  // a static C++ object.
  //

  // Due to a bug in the dynamic linker that was only fixed in Froyo, the
  // static C++ destructor may be called with a value of NULL for the
  // 'this' pointer. As such, any attempt to access any field in the
  // object there will result in a crash. To work-around this, store
  // the key object as a 'static' variable outside of the C++ object.
  static pthread_key_t __cxa_thread_key;

  class CxaThreadKey {
  public:
    // Called at program initialization time, or when the shared library
    // is loaded through dlopen().
    CxaThreadKey() {
      if (pthread_key_create(&__cxa_thread_key, freeObject) != 0) {
        __gabixx::__fatal_error("Can't allocate C++ runtime pthread_key_t");
      }
    }

    // Called at program exit time, or when the shared library is
    // unloaded through dlclose(). See note above.
    ~CxaThreadKey() {
      pthread_key_delete(__cxa_thread_key);
    }

    static __cxa_eh_globals* getFast() {
      void* obj = pthread_getspecific(__cxa_thread_key);
      return reinterpret_cast<__cxa_eh_globals*>(obj);
    }

    static __cxa_eh_globals* getSlow() {
      void* obj = pthread_getspecific(__cxa_thread_key);
      if (obj == NULL) {
        obj = malloc(sizeof(__cxa_eh_globals));
        if (!obj) {
          // Shouldn't happen, but better be safe than sorry.
          __gabixx::__fatal_error(
              "Can't allocate thread-specific C++ runtime info block.");
        }
        memset(obj, 0, sizeof(__cxa_eh_globals));
        pthread_setspecific(__cxa_thread_key, obj);
      }
      return reinterpret_cast<__cxa_eh_globals*>(obj);
    }

  private:
    // Called when a thread is destroyed.
    static void freeObject(void* obj) {
      free(obj);
    }

  };

  // The single static instance, this forces the compiler to register
  // a constructor and destructor for this object in the final library
  // file. They handle the pthread_key_t allocation/deallocation.
  static CxaThreadKey instance;

  _GABIXX_NORETURN void throwException(__cxa_exception *header) {
    __cxa_eh_globals* globals = __cxa_get_globals();
    header->unexpectedHandler = std::get_unexpected();
    header->terminateHandler = std::get_terminate();
    globals->uncaughtExceptions += 1;

    _Unwind_RaiseException(&header->unwindHeader);

    // Should not be here
    call_terminate(&header->unwindHeader);
  }

} // anonymous namespace


namespace __cxxabiv1 {
  __shim_type_info::~__shim_type_info() {
}  // namespace __cxxabiv1

  extern "C" void __cxa_pure_virtual() {
    __gabixx::__fatal_error("Pure virtual function called!");
  }

  extern "C" void __cxa_deleted_virtual() {
    __gabixx::__fatal_error("Deleted virtual function called!");
  }

  extern "C" __cxa_eh_globals* __cxa_get_globals() _GABIXX_NOEXCEPT {
    return CxaThreadKey::getSlow();
  }

  extern "C" __cxa_eh_globals* __cxa_get_globals_fast() _GABIXX_NOEXCEPT {
    return CxaThreadKey::getFast();
  }

  extern "C" void *__cxa_allocate_exception(size_t thrown_size) _GABIXX_NOEXCEPT {
    size_t size = thrown_size + sizeof(__cxa_exception);
    __cxa_exception *buffer = static_cast<__cxa_exception*>(malloc(size));
    if (!buffer) {
      // Since Android uses memory-overcommit, we enter here only when
      // the exception object is VERY large. This will propably never happen.
      // Therefore, we decide to use no emergency spaces.
      __gabixx::__fatal_error("Not enough memory to allocate exception!");
    }

    ::memset(buffer, 0, sizeof(__cxa_exception));
    return buffer + 1;
  }

  extern "C" void __cxa_free_exception(void* thrown_exception) _GABIXX_NOEXCEPT {
    __cxa_exception *exc = static_cast<__cxa_exception*>(thrown_exception) - 1;
    free(exc);
  }

  // Note: Unlike __cxa_allocate_exception, this returns the address
  // of the allocated __cxa_dependent_exception, not the one of any
  // thrown object.
  extern "C" void* __cxa_allocate_dependent_exception () _GABIXX_NOEXCEPT {
    size_t size = sizeof(__cxa_dependent_exception);
    void* ptr = malloc(size);
    if (!ptr) {
      // See comment in __cxa_allocate_exception.
      __gabixx::__fatal_error("Not enough memory to allocate dependent exception!");
    }
    ::memset(ptr, 0, size);
    return ptr;
  }

  // Note: This takes the address of a __cxa_dependent_exception, not
  // the one of a thrown object.
  extern "C" void __cxa_free_dependent_exception(void* dependent_exception)
      _GABIXX_NOEXCEPT {
    free(dependent_exception);
  }

  extern "C" void __cxa_throw(void* thrown_exc,
                              std::type_info* tinfo,
                              void (*dest)(void*)) {
    __cxa_exception* header = static_cast<__cxa_exception*>(thrown_exc) - 1;
    header->referenceCount = 1;
    header->exceptionType = tinfo;
    header->exceptionDestructor = dest;

    header->unwindHeader.exception_class = __gxx_exception_class;
    header->unwindHeader.exception_cleanup = defaultExceptionCleanupFunc;

    throwException(header);
  }

  extern "C" void __cxa_rethrow() {
    __cxa_eh_globals *globals = __cxa_get_globals();
    __cxa_exception* header = globals->caughtExceptions;
    _Unwind_Exception* exception = &header->unwindHeader;
    if (!header) {
      __gabixx::__fatal_error(
          "Attempting to rethrow an exception that doesn't exist!");
    }

    if (__gabixx::__is_our_exception(exception))
      header->handlerCount = -header->handlerCount; // Set rethrow flag
    else
      globals->caughtExceptions = 0;

    throwException(header);
  }

  extern "C" void* __cxa_begin_catch(void* exc) _GABIXX_NOEXCEPT {
    _Unwind_Exception* ue = static_cast<_Unwind_Exception*>(exc);
    __cxa_exception* header =
        reinterpret_cast<__cxa_exception*>(ue + 1) - 1;
    __cxa_eh_globals* globals = __cxa_get_globals();

    if (!__gabixx::__is_our_exception(ue)) {
      // Foreign exception. Don't really know how to handle these.
      __gabixx::__fatal_error("Can't handle non-C++ exception!");
    }

    // Increment the handler count, and remove the rethrow flag.
    if (header->handlerCount < 0)
      header->handlerCount = -header->handlerCount;
    header->handlerCount += 1;

    // Place the exception on top of the stack, if needed.
    globals->uncaughtExceptions -= 1;
    if (header != globals->caughtExceptions) {
      header->nextException = globals->caughtExceptions;
      globals->caughtExceptions = header;
    }

#ifdef __arm__
    _Unwind_Complete(ue);
#endif
    return header->adjustedPtr;
  }

  extern "C" void __cxa_end_catch() _GABIXX_NOEXCEPT {
    __cxa_eh_globals* globals = __cxa_get_globals_fast();
    __cxa_exception *header = globals->caughtExceptions;
    _Unwind_Exception* ue = &header->unwindHeader;

    if (!header)
      return;

    if (ue->exception_class != __gxx_exception_class) {
      // Assume ue->exception_class != __gxx_dependent_exception_class
      // since this was already checked in __cxa_begin_catch().
      // Hence this is was a foreign exception.
      globals->caughtExceptions = 0;
      _Unwind_DeleteException(ue);
      return;
    }

    // Decrement the handler count. If the exception was thrown, the
    // value stored in |handlerCount| is negative.
    bool was_thrown = (header->handlerCount < 0);
    header->handlerCount += was_thrown ? +1 : -1;

    // Remove from stack if there are no more handlers.
    if (header->handlerCount == 0)
      globals->caughtExceptions = header->nextException;

    // Delete if it wasn't thrown.
    if (!was_thrown) {
      if (ue->exception_class == __gxx_dependent_exception_class) {
        // A dependent exception, delete it after getting the address of
        // the primary one.
        __cxa_dependent_exception* dep_header =
            reinterpret_cast<__cxa_dependent_exception*>(header);
        void* primary_thrown = dep_header->primaryException;
        header = reinterpret_cast<__cxa_exception*>(primary_thrown) - 1;
        __cxa_free_dependent_exception(dep_header);
      }

      __cxa_decrement_exception_refcount(header + 1);
    }
  }

  extern "C" void* __cxa_get_exception_ptr(void* thrown_object) _GABIXX_NOEXCEPT {
    __cxa_exception* header =
        reinterpret_cast<__cxa_exception*>(
            reinterpret_cast<_Unwind_Exception *>(thrown_object) + 1) - 1;
    return header->adjustedPtr;
  }

  extern "C" std::type_info* __cxa_current_exception_type() {
    __cxa_eh_globals* globals = __cxa_get_globals_fast();
    if (!globals)
      return NULL;
    __cxa_exception* header = globals->caughtExceptions;
    if (!header)
      return NULL;
    if (!__gabixx::__is_our_exception(&header->unwindHeader)) {
      // Foreign exception has no C++ type.
      return NULL;
    }
    // This works for both __cxa_exception and __cxa_dependent_exception.
    return header->exceptionType;
  }

  extern "C" bool __cxa_uncaught_exception() _GABIXX_NOEXCEPT {
    __cxa_eh_globals* globals = __cxa_get_globals();
    if (globals == NULL)
      return false;
    return globals->uncaughtExceptions == 0;
  }

  extern "C" void __cxa_decrement_exception_refcount(void* thrown_object)
      _GABIXX_NOEXCEPT {
    if (thrown_object != NULL) {
      __cxa_exception* header =
          reinterpret_cast<__cxa_exception*>(thrown_object) - 1;
      if (__sync_sub_and_fetch(&header->referenceCount, 1) == 0) {
        if (header->exceptionDestructor) {
          try {
            header->exceptionDestructor(thrown_object);
          } catch (...) {
            __gabixx::__fatal_error("Exception destructor has thrown!");
          }
        }
        __cxa_free_exception(thrown_object);
      }
    }
  }

  extern "C" void __cxa_increment_exception_refcount(void* thrown_object)
      _GABIXX_NOEXCEPT {
    if (thrown_object != NULL) {
      __cxa_exception* header =
          reinterpret_cast<__cxa_exception*>(thrown_object) - 1;
      __sync_add_and_fetch(&header->referenceCount, 1);
    }
  }

  // Used internally by __cxa_rethrow_primary_exception to cleanup
  // a __cxx_dependent_exception. This is normally only called when
  // a foreign (non-C++) exception handler calls _Unwind_DeleteException().
  static void dependent_exception_cleanup(_Unwind_Reason_Code reason,
                                          _Unwind_Exception* ue) {
    __cxa_dependent_exception* dep_header =
        reinterpret_cast<__cxa_dependent_exception*>(ue + 1) - 1;
    if (reason != _URC_FOREIGN_EXCEPTION_CAUGHT)
      __gabixx::__terminate(dep_header->terminateHandler);

    __cxa_decrement_exception_refcount(dep_header->primaryException);
    __cxa_free_dependent_exception(dep_header);
  }

  // Rethrow a primary native C++ exception in the current thread.
  // |thrown_object| must correspond to a __cxa_exception object and
  // nothing else. This creates a __cxa_dependent_exception object to
  // wrap it. If this function returns, an error occured, and the client
  // must call std::terminate().
  extern "C" void __cxa_rethrow_primary_exception(void* thrown_object) {
    if (thrown_object == NULL) {
      // caller should call std::terminate();
      return;
    }

    __cxa_exception* header =
        reinterpret_cast<__cxa_exception*>(thrown_object) - 1;

    if (header->unwindHeader.exception_class !=
        __gxx_exception_class) {
      __gabixx::__fatal_error("Can't rethrow non-primary C++ exception!");
    }

    __cxa_dependent_exception* dep_header =
        static_cast<__cxa_dependent_exception*>(
            __cxa_allocate_dependent_exception());

    dep_header->primaryException = thrown_object;
    __cxa_increment_exception_refcount(thrown_object);

    dep_header->exceptionType = header->exceptionType;
    dep_header->unexpectedHandler = header->unexpectedHandler;
    dep_header->terminateHandler = header->terminateHandler;
    dep_header->unwindHeader.exception_class = __gxx_dependent_exception_class;
    dep_header->unwindHeader.exception_cleanup = dependent_exception_cleanup;

    int reason = _Unwind_RaiseException(&dep_header->unwindHeader);

    printf("_UnwindRaiseException() returned %d\n", reason);

    // If _Unwind_RaiseException returns, it's a failed throw.
    __cxa_begin_catch(&dep_header->unwindHeader);
    std::terminate();
  }

  // Returns a pointer to the top-most native thrown object, if any. This
  // also increments the corresponding __cxa_exception's reference count
  // field before returning.
  extern "C" void* __cxa_current_primary_exception() _GABIXX_NOEXCEPT {
    __cxa_eh_globals* globals = __cxa_get_globals_fast();
    if (!globals)
      return NULL;  // No exception was ever thrown on this thread.

    __cxa_exception* header = globals->caughtExceptions;
    if (header == NULL)
      return NULL;  // No exception is thrown on this thread.

    void* thrown_object;

    if (header->unwindHeader.exception_class == __gxx_exception_class) {
      thrown_object = header + 1;
    } else if (header->unwindHeader.exception_class ==
        __gxx_dependent_exception_class) {
      // Extract primary exception from dependent one.
      __cxa_dependent_exception* dep_header =
          reinterpret_cast<__cxa_dependent_exception*>(header);
      thrown_object = dep_header->primaryException;
    } else {
      // A foreign exception can't be wrapped.
      return NULL;
    }
    __cxa_increment_exception_refcount(thrown_object);
    return thrown_object;
  }

} // namespace __cxxabiv1
