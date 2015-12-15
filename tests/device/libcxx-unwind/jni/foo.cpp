//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <map>

int main() {
  typedef std::pair<const int, double> V;
  V ar[] = {
      V(1, 1.5), V(2, 2.5), V(3, 3.5), V(4, 4.5),
      V(5, 5.5), V(7, 7.5), V(8, 8.5),
  };
  std::map<int, double> m(ar, ar + sizeof(ar) / sizeof(ar[0]));
  m.at(1) = -1.5;
  try {
    m.at(6);
    assert(0);
  } catch (std::out_of_range &) {
  }
}
