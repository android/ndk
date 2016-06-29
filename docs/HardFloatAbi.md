ARM Hard Float ABI Removal
==========================

We're removing support for the armeabi-v7a-hard ABI. Anyone targeting this ABI
should target armeabi-v7a instead.

Why?

1. People often use it because they think it's required to get
   floating point instructions, which is incorrect.
2. Most of the cases where people do see a performance improvement
   here are likely bugs/missed optimizations in either the compiler or
   the user's code, and hiding those means they won't get fixed.
3. It's full of bugs and fixing them and keeping them fixed isn't
   worth the benefit, especially in a world where you can avoid the
   problem entirely by making your app 64-bit.

armeabi-v7a-hard is not a real ABI. No devices are built with this.
Hard float code generation is already used in the armeabi-v7a ABI.
The "hard float" variant only changes the function call ABI. That is,
the floating point registers are used to pass floating point
arguments rather than the integer registers.

This ABI should only be beneficial if the workload consists of many
math function calls, the math function is cheap enough that the call
itself is expensive, and it's not being inlined. The bug here is that
the call is not being inlined.

There is a cost to keeping this functionality. The way this "ABI"
works is by having an `__attribute__((pcs("aapcs")))` on every
function that is part of the Android ABI and has floating point
parameters (anything that is in the system libraries or zygote). If we
miss one (and we do, often), the function will not be called
correctly.

I should note that the GCC docs explicitly state that linking
-mfloat-abi=softfp and -mfloat-abi=hard is invalid:
https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html

> Note that the hard-float and soft-float ABIs are not
> link-compatible; you must compile your entire program with the same
> ABI, and link with a compatible set of libraries.
