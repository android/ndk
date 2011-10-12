/* This file is here to check for the following bug report:
 *   http://code.google.com/p/android/issues/detail?id=16010
 */
#include <stdio.h>

class A {
public:
  static A* instance() {
    static A _instance;
    return &_instance;
  }
};

int main() {
    A* first = A::instance();
    A* second = A::instance();

    if (first != second) {
        fprintf(stderr, "ERROR: instance() returned two distinct addresses: %p %p\n", first, second);
        return 1;
    }
    printf("OK: instance() returned the same address twice: %p\n", first);
   return 0;
}
