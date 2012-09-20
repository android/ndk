#include <cstdio>
#include "liba.h"
int global = 0x42;
void func()
{
    printf("global = 0x%x (%p)\n", global, &global);
}

