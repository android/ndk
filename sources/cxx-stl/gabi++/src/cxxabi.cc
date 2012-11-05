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
#include <cxxabi.h>
#include <exception>
#include "helper_func_internal.h"

namespace {

  using namespace __cxxabiv1;

  bool isOurCxxException(uint64_t exc) {
    // Compatible with GNU
    return exc == __gxx_exception_class;
  }

  void defaultExceptionCleanupFunc(_Unwind_Reason_Code reason, _Unwind_Exception* exc) {
    __cxa_free_exception(exc);
  }

  __cxa_thread_info* getThreadInfo() {
    // Only single thread for now
    static __cxa_thread_info info;
    return &info;
  }

  void throwException(__cxa_exception *header) {
    __cxa_thread_info *info = getThreadInfo();
    header->unexpectedHandler = info->unexpectedHandler;
    if (!header->unexpectedHandler) {
      header->unexpectedHandler = std::current_unexpected_fn;
    }
    header->terminateHandler = info->terminateHandler;
    if (!header->terminateHandler) {
      header->terminateHandler = std::current_terminate_fn;
    }
    info->globals.uncaughtExceptions += 1;

    _Unwind_Reason_Code ret = _Unwind_RaiseException(&header->unwindHeader);

    // Should not be here
    call_terminate(&header->unwindHeader);
  }

  unsigned char emergency_buffer[1024*16];  // 1KB*16
  bool buffer_allocated[16];

  void* emergency_malloc(size_t size) {
    if (size > 1024) {
      return NULL;
    }

    __cxa_thread_info* info = getThreadInfo();
    if (info->emergencyBuffersHeld > 3) {
      return 0; // Per thread get at most 4
    }
    int number = -1;
    for (int i = 0; i < 16; ++i) {
      if (!buffer_allocated[i]) {
        number = i;
        buffer_allocated[i] = true;
        break;
      }
    }
    assert(number != -1 && "No enough emergency space left.");

    info->emergencyBuffersHeld += 1;
    return emergency_buffer + (1024 * number);
  }

  void emergency_malloc_free(void* ptr) {
    int number = -1;
    for (int i = 0; i < 16; ++i) {
      if (ptr == static_cast<void*>(emergency_buffer + (1024*i))) {
        number = i;
        break;
      }
    }
    assert(number != -1 && "Cannot find this allocated space. What wrong?");

    memset(ptr, 0, 1024);
    buffer_allocated[number] = false;
    getThreadInfo()->emergencyBuffersHeld -= 1;
  }

} // anonymous namespace


namespace __cxxabiv1 {

  extern "C" void __cxa_pure_virtual() {
    fprintf(stderr, "Pure virtual function called. Terminate!\n");
    std::terminate();
  }

  extern "C" __cxa_eh_globals* __cxa_get_globals() {
    return &getThreadInfo()->globals;
  }

  extern "C" __cxa_eh_globals* __cxa_get_globals_fast() {
    // TODO: Implement a fast version
    return __cxa_get_globals();
  }


  extern "C" void *__cxa_allocate_exception(size_t thrown_size) {
    size_t size = thrown_size + sizeof(__cxa_exception);
    __cxa_exception *buffer = static_cast<__cxa_exception*>(malloc(size));
    if (!buffer) {
      buffer = static_cast<__cxa_exception*>(emergency_malloc(size));
      if (!buffer) {
        std::terminate();
      }
    }

    memset(buffer, 0, sizeof(__cxa_exception));
    return buffer + 1;
  }

  extern "C" void __cxa_free_exception(void* thrown_exception) {
    __cxa_exception *exc = static_cast<__cxa_exception*>(thrown_exception)-1;

    if (exc->exceptionDestructor) {
      try {
        exc->exceptionDestructor(thrown_exception);
      } catch (...) {
        std::terminate(); // Failed when exception destructing
      }
    }

    if (((void*)exc > emergency_buffer) &&
        ((void*)exc < (emergency_buffer + sizeof(emergency_buffer)))) {
      emergency_malloc_free(exc);
    } else {
      free(exc);
    }
  }


  extern "C" void __cxa_throw(void* thrown_exc,
                              std::type_info* tinfo,
                              void (*dest)(void*)) {
    __cxa_exception* header = static_cast<__cxa_exception*>(thrown_exc)-1;
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
      fprintf(stderr, "Attempting to rethrow an exception that doesn't exist!\n");
      std::terminate();
    }

    if (isOurCxxException(exception->exception_class)) {
      header->handlerCount = -header->handlerCount; // Set rethrow flag
    } else {
      globals->caughtExceptions = 0;
    }

    throwException(header);
  }


  extern "C" void* __cxa_begin_catch(void* exc) {
    _Unwind_Exception *exception = static_cast<_Unwind_Exception*>(exc);
    __cxa_exception* header = reinterpret_cast<__cxa_exception*>(exception+1)-1;
    __cxa_eh_globals* globals = __cxa_get_globals();

    if (!isOurCxxException(exception->exception_class)) {
      if (globals->caughtExceptions) {
        std::terminate();
      }
    }

    // Check rethrow flag
    header->handlerCount = (header->handlerCount < 0) ?
      (-header->handlerCount+1) : (header->handlerCount+1);

    if (header != globals->caughtExceptions) {
      header->nextException = globals->caughtExceptions;
      globals->caughtExceptions = header;
    }
    globals->uncaughtExceptions -= 1;

    return header->adjustedPtr;
  }

  extern "C" void __cxa_end_catch() {
    __cxa_eh_globals *globals = __cxa_get_globals_fast();
    __cxa_exception *header = globals->caughtExceptions;
    _Unwind_Exception* exception = &header->unwindHeader;

    if (!header) {
      return;
    }

    if (!isOurCxxException(exception->exception_class)) {
      globals->caughtExceptions = 0;
      _Unwind_DeleteException(exception);
      return;
    }

    int count = header->handlerCount;
    if (count < 0) { // Rethrow
      if (++count == 0) {
        globals->caughtExceptions = header->nextException;
      }
    } else if (--count == 0) {
      globals->caughtExceptions = header->nextException;
      __cxa_free_exception(header+1);
      return;
    } else if (count < 0) {
      std::terminate(); // Some bug happens here
    }

    header->handlerCount = count;
  }

  extern "C" void* __cxa_get_exception_ptr(void* exceptionObject) {
    __cxa_exception* header =
      reinterpret_cast<__cxa_exception*>(
        reinterpret_cast<_Unwind_Exception *>(exceptionObject)+1)-1;
    return header->adjustedPtr;
  }

} // namespace __cxxabiv1
