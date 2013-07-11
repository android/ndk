#include <stdio.h>
#include "header.h"

void foo(union u *ptr, int flag)
{
    union u local;
    local.i = ptr->i;

    printf("Calling printf to trash r0..r3\n");
    /*
     * Clang 3.1 generates the following instructions to store flag into
     * local.s.x. Because str is paired with ldrh, the high half-word in r0 is
     * uninitialized and the corresponding positions in local.s.i will be
     * clobbered.
     *
     * 1a:   f8bd 0004       ldrh.w  r0, [sp, #4]
     * 1e:   f005 0101       and.w   r1, r5, #1
     * 22:   f020 0001       bic.w   r0, r0, #1
     * 26:   4308            orrs    r0, r1
     * 28:   9001            str     r0, [sp, #4]
     */
    local.s.x = flag;
    ptr->i = local.i;
}
