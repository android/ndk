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
  std::cout.imbue(std::locale("C"));
  std::cout << "Hello World\n" << std::endl;
  return 0;
}
