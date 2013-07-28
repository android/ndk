// Copyright (C) 2013 The Android Open Source Project
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

#ifndef __GABIXX_CONFIG_H__
#define __GABIXX_CONFIG_H__

// Used to tag functions that never return.
// IMPORTANT: This must appear at the left of function definitions,
// as in:
//  _GABIXX_NORETURN <return-type> <name>(....) { ... }
#define _GABIXX_NORETURN  __attribute__((__noreturn__))

// Use _GABIXX_NOEXCEPT to use the equivalent of the C++11 noexcept
// qualifier at the end of function declarations.
//
// _GABIXX_NOEXCEPT_() only in C++11 mode to use the noexcept() operator.
#if __cplusplus >= 201103L
#  define _GABIXX_NOEXCEPT noexcept
#  define _GABIXX_NOEXCEPT_(x) noexcept(x)
#else
#  define _GABIXX_NOEXCEPT throw()
#  define _GABIXX_NOEXCEPT_(x) /* nothing */
#endif

// Use _GABIXX_ALWAYS_INLINE to declare a function that shall always be
// inlined. Note that the always_inline doesn't make a function inline
// per se.
#define _GABIXX_ALWAYS_INLINE \
  inline __attribute__((__always_inline__))

// _GABIXX_HAS_EXCEPTIONS will be 1 if the current source file is compiled
// with exceptions support, or 0 otherwise.
#if !defined(__clang__) && !defined(__has_feature)
#define __has_feature(x) 0
#endif

#if (defined(__clang__) && __has_feature(cxx_exceptions)) || \
    (defined(__GNUC__) && defined(__EXCEPTIONS))
#define _GABIXX_HAS_EXCEPTIONS 1
#else
#define _GABIXX_HAS_EXCEPTIONS 0
#endif

// TODO(digit): Use __atomic_load_acq_rel when available.
#define __gabixx_sync_load(address)  \
    __sync_fetch_and_add((address), (typeof(*(address)))0)

// Clang provides __sync_swap(), but GCC does not.
// IMPORTANT: For GCC, __sync_lock_test_and_set has acquire semantics only
// so an explicit __sync_synchronize is needed to ensure a full barrier.
// TODO(digit): Use __atomic_swap_acq_rel when available.
#if defined(__clang__)
#  define __gabixx_sync_swap(address,value)  __sync_swap((address),(value))
#else
#  define __gabixx_sync_swap(address, value)  \
  __extension__ ({ \
    typeof(*(address)) __ret = __sync_lock_test_and_set((address),(value)); \
    __sync_synchronize(); \
    __ret; \
  })
#endif

#endif  // __GABIXX_CONFIG_H__
