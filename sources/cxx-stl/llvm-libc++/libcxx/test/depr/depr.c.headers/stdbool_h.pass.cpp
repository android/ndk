//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// test <stdbool.h>

#include <stdbool.h>

#ifndef __bool_true_false_are_defined
#error __bool_true_false_are_defined not defined
#endif

#if !defined(__clang__)
// GCC defines bool, true, and false in lib/gcc/arm-linux-androideabi/4.8/include/stdbool.h
#else

#ifdef bool
#error bool should not be defined
#endif

#ifdef true
#error true should not be defined
#endif

#ifdef false
#error false should not be defined
#endif

#endif

int main()
{
}
