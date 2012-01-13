/* This file should be compiled without exceptions and with RTTI */
#ifdef __EXCEPTIONS
#error This source file SHOULD NOT be built with -fexceptions!
#endif

#include <typeinfo>
#include <stdio.h>

class Foo { int x; };

int main(void) {
    printf("%p\n", &typeid(Foo)); // will fail without -frtti
    return 0;
}
