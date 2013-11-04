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

#include <exception>
#include <typeinfo>

#include "cxxabi_defines.h"

namespace std {

#if !defined(GABIXX_LIBCXX)
exception::exception() _GABIXX_NOEXCEPT {
}
#endif // !defined(GABIXX_LIBCXX)

exception::~exception() _GABIXX_NOEXCEPT {
}

const char* exception::what() const _GABIXX_NOEXCEPT {
  return "std::exception";
}

#if !defined(GABIXX_LIBCXX)
bad_exception::bad_exception() _GABIXX_NOEXCEPT {
}

bad_exception::~bad_exception() _GABIXX_NOEXCEPT {
}

const char* bad_exception::what() const _GABIXX_NOEXCEPT {
  return "std::bad_exception";
}
#endif // !defined(GABIXX_LIBCXX)

bad_cast::bad_cast() _GABIXX_NOEXCEPT {
}

bad_cast::~bad_cast() _GABIXX_NOEXCEPT {
}

const char* bad_cast::what() const _GABIXX_NOEXCEPT {
  return "std::bad_cast";
}

bad_typeid::bad_typeid() _GABIXX_NOEXCEPT {
}

bad_typeid::~bad_typeid() _GABIXX_NOEXCEPT {
}

const char* bad_typeid::what() const _GABIXX_NOEXCEPT {
  return "std::bad_typeid";
}

#ifndef GABIXX_LIBCXX

// std::exception_ptr

using namespace __cxxabiv1;

exception_ptr::exception_ptr(const exception_ptr& other) _GABIXX_NOEXCEPT
    : _ptr(other._ptr) {
  __cxa_increment_exception_refcount(_ptr);
}

exception_ptr& exception_ptr::operator=(const exception_ptr& other)
      _GABIXX_NOEXCEPT {
  if (_ptr != other._ptr) {
    __cxa_increment_exception_refcount(other._ptr);
    __cxa_decrement_exception_refcount(_ptr);
    _ptr = other._ptr;
  }
  return *this;
}

exception_ptr::~exception_ptr() _GABIXX_NOEXCEPT {
  __cxa_decrement_exception_refcount(_ptr);
};

// std::current_exception()

exception_ptr current_exception() _GABIXX_NOEXCEPT {
  return exception_ptr(__cxa_current_primary_exception());
}

// std::rethrow_exception()

_GABIXX_NORETURN void rethrow_exception(exception_ptr ptr) {
  __cxa_rethrow_primary_exception(ptr._ptr);
  __gabixx::__fatal_error("Can't rethrow null exception!");
}

#endif // !defined(GABIXX_LIBCXX)

}  // namespace std
