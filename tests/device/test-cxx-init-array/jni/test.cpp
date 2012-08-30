#include <stdio.h>
#include "global_ctor.h"

GlobalCtor Obj;

int main(void)
{
    if (Obj.IsInitialized())
    {
        puts("Passed");
        return 0;
    }
    else
    {
        puts("ERROR: Global variables are NOT initialized.");
        return 1;
    }
}
