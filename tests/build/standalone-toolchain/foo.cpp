#include <stdio.h>

#include <string>

void foo(const std::string& s) {
  printf("%s\n", s.c_str());
}
