// Copyright (C) 2011 The Android Open Source Project
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
//

#include <stdlib.h>
#include <new>

using std::new_handler;
namespace {
  new_handler cur_handler;
}

namespace std {

#if !defined(GABIXX_LIBCXX)
  const nothrow_t nothrow = {};
#endif  // !defined(GABIXX_LIBCXX)

  bad_alloc::bad_alloc() throw() {
  }

  bad_alloc::~bad_alloc() throw() {
  }

  const char* bad_alloc::what() const throw() {
    return "std::bad_alloc";
  }

  new_handler set_new_handler(new_handler next_handler) throw() {
    new_handler old_handler = cur_handler;
    cur_handler = next_handler;
    return old_handler;
  }

} // namespace std

__attribute__ ((weak))
void* operator new(std::size_t size) throw(std::bad_alloc) {
  void* space;
  do {
    space = malloc(size);
    if (space) {
      return space;
    }
    new_handler handler = cur_handler;
    if (handler == NULL) {
      throw std::bad_alloc();
    }
    handler();
  } while (space == 0);
}

__attribute__ ((weak))
void* operator new(std::size_t size, const std::nothrow_t& no) throw() {
  try {
    return ::operator new(size);
  } catch (const std::bad_alloc&) {
    return 0;
  }
}

__attribute__ ((weak))
void* operator new[](std::size_t size) throw(std::bad_alloc) {
  return ::operator new(size);
}

__attribute__ ((weak))
void* operator new[](std::size_t size, const std::nothrow_t& no) throw() {
  return ::operator new(size, no);
}
