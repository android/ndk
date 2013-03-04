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
  // TODO(ajwong): This imbue should not be required. There is a problem with
  // the module initialization code that causes the default "C" locale to not
  // be created correctly. The manual imbue allows us to proceed with
  // development for now.
  std::cout.imbue(std::locale("C"));
  std::cout << "Hello World" << std::endl;
  return 0;
}
