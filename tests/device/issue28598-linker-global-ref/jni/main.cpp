#include <cstdio>
#include "liba.h"
int *global_ptr = &global; // Without this line, everything is fine
int main()
{
    func();
    printf("global = 0x%x (%p)%s\n", global, &global, ((global != 0x42)? ", ERROR!!!":""));
    return global != 0x42;
}

