#include <stdio.h>
#include "header.h"

int main()
{
    union u u;

    u.i = 0xfedcba98;
    printf("u.i = %08x\n", u.i);
    foo(&u, 1);
    printf("u.i = %08x (expecting fedcba99)\n", u.i);

    return (u.i == 0xfedcba99)? 0 : 1;
}
