// This assembler file contains instructions like ssax
// that were not assembler properly with NDKr5b's gas version.
//

///////        unittest.s  ////////
@ For little endian
@.arch armv7a

.align  2
armSP_FFTFwd_CToC_SC16_Radix4_fs_OutOfPlace_unsafe:
push    {r4, lr}
sub     sp, sp, #32     @ 0x20
mov     ip, #4  @ 0x4
lsr     lr, r6, #2
str     ip, [sp, #4]
str     lr, [sp]
str     r5, [sp, #16]
lsl     ip, lr, #2
lsl     r3, lr, #3
add     r3, r3, ip
rsb     r3, r3, #8      @ 0x8
str     r1, [sp, #20]
ldrd    r4, [r0], ip
ldrd    r6, [r0], ip
ldrd    r8, [r0], ip
ldrd    sl, [r0], r3
strd    r2, [sp, #8]
subs    lr, lr, #2      @ 0x2
str     lr, [sp, #24]
sadd16  r2, r4, r8
ssub16  r1, r4, r8
sadd16  r3, r6, sl
ssub16  lr, r6, sl
sadd16  r4, r2, r3
ssub16  r8, r2, r3
ssax    r6, r1, lr
sasx sl, r1, lr
sadd16  r2, r5, r9
ssub16  r1, r5, r9
sadd16  r3, r7, fp
ssub16  lr, r7, fp
sadd16  r5, r2, r3
ssub16  r9, r2, r3
ldrd    r2, [sp, #8]
ssax    r7, r1, lr
sasx    fp, r1, lr
strd    r4, [r2], ip
strd    r6, [r2], ip
strd    r8, [r2], ip
strd    sl, [r2], r3
ldr     lr, [sp, #24]
bgt     armSP_FFTFwd_CToC_SC16_Radix4_fs_OutOfPlace_unsafe+0x30
ldrd    r6, [sp]
ldr     r1, [sp, #20]
sub     r0, r2, ip
ldr     r2, [sp, #16]
add     sp, sp, #32     @ 0x20
pop     {r4, pc}
