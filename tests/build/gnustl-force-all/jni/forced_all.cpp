/* This file should be compiled with exceptions and without RTTI */
#ifndef __EXCEPTIONS
#error This source file SHOULD be built with -fexceptions!
#endif

#include <typeinfo>
#include <stdio.h>

class Foo { int x; };

int main(void) {
    printf("%p\n", &typeid(Foo)); // will fail without -frtti
    return 0;
}
