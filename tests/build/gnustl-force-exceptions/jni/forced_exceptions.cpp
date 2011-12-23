// This file should FAIL to build iff exceptions are enabled and RTTI is disabled
#ifndef __EXCEPTIONS
int main(void) { return 0; }
#else // !exceptions
#include <typeinfo>
#include <stdio.h>

class Foo { int x; };

int main(void) {
    printf("%p\n", typeid(Foo)); // will fail without -frtti
    return 0;
}
#endif
