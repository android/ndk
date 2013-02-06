#include <stdio.h>

#include "foo.h"
#include "bar.h"

int main() {
  int x = 10;
  printf("foo(%d) = %d\n", x, foo(x));
  printf("bar(%d) = %d\n", x, bar(x));
  return 0;
}
