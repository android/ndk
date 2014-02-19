// -*- C++ -*-
//===-------------------- support/android/wchar_support.c ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <locale>

int main(void) {
  // TODO(digit): This imbue is required, otherwise a crash will happen
  // at runtime, with the following stack trace:
  //
  //   #0  0x00069ae0 in std::(anonymous namespace)::default_terminate () at jni/../../../../gabi++/src/terminate.cc:57
  //   #1  0x00069e48 in std::terminate () at jni/../../../../gabi++/src/terminate.cc:123
  //   #2  0x00067d44 in __cxxabiv1::call_terminate (unwind_exception=0x1827470) at jni/../../../../gabi++/src/helper_func_internal.cc:58
  //   #3  0x00066bf8 in (anonymous namespace)::throwException (header=0x1827438) at jni/../../../../gabi++/src/cxxabi.cc:126
  //   #4  0x00066e64 in __cxxabiv1::__cxa_throw (thrown_exc=0x18274c8, tinfo=0x7fa00, dest=0x69c10 <std::bad_cast::~bad_cast()>)
  //       at jni/../../../../gabi++/src/cxxabi.cc:190
  //   #5  0x00010be0 in std::__1::locale::__imp::use_facet (this=0x80b28, id=28) at jni/../../../libcxx/src/locale.cpp:426
  //   #6  0x00010f50 in std::__1::locale::use_facet (this=0xbe980ab4, x=...) at jni/../../../libcxx/src/locale.cpp:568
  //   #7  0x00009e88 in use_facet<std::__1::ctype<char> > (__l=<optimized out>) at jni/../../../libcxx/include/__locale:172
  //   #8  widen (__c=10 '\n', this=0x80084) at jni/../../../libcxx/include/ios:726
  //   #9  std::__1::endl<char, std::__1::char_traits<char> > (__os=...) at jni/../../../libcxx/include/ostream:1181
  //   #10 0x00009ad0 in operator<< (
  //   __pf=0x9e20 <std::__1::endl<char, std::__1::char_traits<char> >(std::__1::basic_ostream<char, std::__1::char_traits<char> >&)>, this=0x80080) at jni/../../../libcxx/include/ostream:306
  //   #11 main () at jni/test_1.cc:22
  //
  // The issue seems to be that on #9, the id retrieved through
  // locale::id::__get() reaches 28, which is an invalid index in the
  // facets_ vector defined in locale.cpp:124 and populated in
  // locale::__imp::__imp(size_t) in the same source file.
  //
  // Not sure if a libc++ bug or something else. Investigate.
  std::cout.imbue(std::locale("C"));
  std::cout << "Hello World" << std::endl;
  return 0;
}
