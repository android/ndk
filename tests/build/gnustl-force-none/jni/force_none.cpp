// This program should FAIL to build iff exceptions and RTTI are disabled.
#ifdef __EXCEPTIONS
// can't fail to build here
int main(void) { return 0; }
#else // !exceptions
#include <typeinfo>
#include <stdio.h>

class Foo { int x; };

int main(void) {
    printf("%p\n", typeid(Foo)); // will fail with RTTI
    return 0;
}
#endif
