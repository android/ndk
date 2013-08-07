#include <stdio.h>
int main()
{
    printf ("Hello 1\n");
    try {
        throw 20;
    } catch(...) {
        printf ("catch\n");
    }
    printf ("Hello 2\n");
    return 0;
}
