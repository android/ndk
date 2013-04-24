/* This simple file is used to check that compiler flags are properly
 * set when using the NDK build system. Look at Android.mk to see how
 * the various CHECK_XXX macros are supposed to be defined.
 */
int main(void)
{
#if defined(CHECK_THUMB)
#  ifndef __arm__
#    error "This source file should be compiled with an ARM toolchain"
#  endif
#  ifndef __thumb__
#    error "This source file should be built in thumb mode!"
#  endif
#elif defined(CHECK_THUMB2)
#  ifndef __arm__
#    error "This source file should be compiled with an ARM toolchain"
#  endif
#  ifndef __thumb2__
#    error "This source file should be built in thumb2 mode!"
#  endif
#elif defined(CHECK_ARM)
#  ifndef __arm__
#    error "This source file should be compiled with an ARM toolchain"
#  endif
#  if defined(__thumb__) || defined(__thumb2__)
#    error "This source file should be compiled to 32-bit ARM instructions"
#  endif
#elif defined(CHECK_X86)
#  ifndef __i386__
#    error "This source file should be compiled with an x86 toolchain"
#  endif
#elif defined(CHECK_MIPS)
#  ifndef __mips__
#    error "This source file should be compiled with a MIPS toolchain"
#  endif
#else
#  error "This unit test is broken!"
#endif

#if defined(CHECK_NEON)
#  ifndef __ARM_NEON__
#    error "This source file should be compiled with NEON support!"
#  endif
#else
#  ifdef __ARM_NEON__
#    error "This source file should be compiled without NEON support!"
#  endif
#endif

#ifndef __ANDROID__
#  error "This toolchain doesn't define the __ANDROID__ built-in macro!"
#endif
    return 0;
}
