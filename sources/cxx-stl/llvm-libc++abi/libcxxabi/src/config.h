//===----------------------------- config.h -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//
//  Defines macros used within the libc++abi project.
//
//===----------------------------------------------------------------------===//


#ifndef LIBCXXABI_CONFIG_H
#define LIBCXXABI_CONFIG_H

#include <unistd.h>

#if defined(_POSIX_THREADS) && _POSIX_THREADS > 0
#  define LIBCXXABI_SINGLE_THREADED 0
#else
#  define LIBCXXABI_SINGLE_THREADED 1
#endif

// Set this in the CXXFLAGS when you need it, because otherwise we'd have to
// #if !defined(__linux__) && !defined(__APPLE__) && ...
// and so-on for *every* platform.
#ifndef LIBCXXABI_BARE_METAL
#  define LIBCXXABI_BARE_METAL 0
#endif

// -- BEGIN Taken from gabixx_config.h for gcc compat
// Clang provides __sync_swap(), but GCC does not.
// IMPORTANT: For GCC, __sync_lock_test_and_set has acquire semantics only
// so an explicit __sync_synchronize is needed to ensure a full barrier.
// TODO(digit): Use __atomic_swap_acq_rel when available.
#if !defined(__clang__)
#  define __sync_swap(address, value)  \
  __extension__ ({ \
    __typeof__(*(address)) __ret = __sync_lock_test_and_set((address),(value)); \
    __sync_synchronize(); \
    __ret; \
  })
#endif
// -- END Taken from gabixx_config.h for gcc compat


#endif // LIBCXXABI_CONFIG_H
