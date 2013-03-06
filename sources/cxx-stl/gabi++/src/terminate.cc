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

#include <cstdlib>
#include <cxxabi.h>
#include <exception>

namespace std {

  namespace {
    // The default std::terminate() implementation will crash the process.
    // This is done to help debugging, i.e.:
    //   - When running the program in a debugger, it's trivial to get
    //     a complete stack trace explaining the failure.
    //
    //   - Otherwise, the default SIGSEGV handler will generate a stack
    //     trace in the log, that can later be processed with ndk-stack
    //     and other tools.
    //
    //   - Finally, this also works when a custom SIGSEGV handler has been
    //     installed. E.g. when using Google Breakpad, the termination will
    //     be recorded in a Minidump, which contains a stack trace to be
    //     later analyzed.
    //
    // The C++ specification states that the default std::terminate()
    // handler is library-specific, even though most implementation simply
    // choose to call abort() in this case.
    //
    void default_terminate(void) {
      // The crash address is just a "magic" constant that can be used to
      // identify stack traces (like 0xdeadbaad is used when heap corruption
      // is detected in the C library). 'cab1' stands for "C++ ABI" :-)
      *(reinterpret_cast<char*>(0xdeadcab1)) = 0;

      // should not be here, but just in case.
      abort();
    }

    terminate_handler current_terminate_fn = default_terminate;
    unexpected_handler current_unexpected_fn = terminate;
  }  // namespace

#if !defined(GABIXX_LIBCXX)
  exception::exception() throw() {
  }
#endif // !defined(GABIXX_LIBCXX)

  exception::~exception() throw() {
  }

  const char* exception::what() const throw() {
    return "std::exception";
  }

#if !defined(GABIXX_LIBCXX)
  bad_exception::bad_exception() throw() {
  }

  bad_exception::~bad_exception() throw() {
  }

  const char* bad_exception::what() const throw() {
    return "std::bad_exception";
  }
#endif // !defined(GABIXX_LIBCXX)

  bad_cast::bad_cast() throw() {
  }

  bad_cast::~bad_cast() throw() {
  }

  const char* bad_cast::what() const throw() {
    return "std::bad_cast";
  }

  bad_typeid::bad_typeid() throw() {
  }

  bad_typeid::~bad_typeid() throw() {
  }

  const char* bad_typeid::what() const throw() {
    return "std::bad_typeid";
  }

  terminate_handler get_terminate() {
    return current_terminate_fn;
  }

  terminate_handler set_terminate(terminate_handler f) {
    terminate_handler tmp = current_terminate_fn;
    current_terminate_fn = f;
    return tmp;
  }

  void terminate() {
    try {
      current_terminate_fn();
      abort();
    } catch (...) {
      abort();
    }
  }

  unexpected_handler get_unexpected() {
    return current_unexpected_fn;
  }

  unexpected_handler set_unexpected(unexpected_handler f) {
    unexpected_handler tmp = current_unexpected_fn;
    current_unexpected_fn = f;
    return tmp;
  }

  void unexpected() {
    current_unexpected_fn();
    terminate();
  }

  bool uncaught_exception() throw() {
    using namespace __cxxabiv1;

    __cxa_eh_globals* globals = __cxa_get_globals();
    return globals->uncaughtExceptions != 0;
  }

} // namespace std
