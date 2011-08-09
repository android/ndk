/* The purpose of this test is to check that the target sysroot directory
 * appears _after_ the ones in LOCAL_C_INCLUDES, this allows us to
 * override certain headers.
 *
 * See http://code.google.com/p/android/issues/detail?id=18540
 */
#include <stdio.h>

/* This header should come from our $(LOCAL_PATH)/include directory */
#include <math.h>

#ifndef CUSTOM_MATH_H_WAS_INCLUDED
#error THERE IS SOMETHING WRONG HERE !!
#endif

int foo(void)
{
    return 42;
}
