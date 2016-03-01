Changelog
=========

Clang
-----

 * Clang has been updated to 3.8svn (r256229, build 2481030).
     * Note that Clang packaged in the Windows 64 NDK is actually 32-bit.

GCC
---

 * Synchronized with the ChromeOS GCC. TODO(cmtice): Anything to mention here?

NDK
---

 * Exception handling when using `c++_shared` on ARM32 now works. The unwinder
   will now be linked into each linked object rather than into libc++ itself.

Binutils
--------

 * TODO: Update so we can get the fix for the clang+x86+dyncast KI.

GDB
---

 * TODO(jmgao): Any upcoming release to sync?

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * x86 ASAN does not currently work. See discussion on
   https://android-review.googlesource.com/#/c/186276/
 * The combination of Clang, x86, `stlport_static`, and optimization levels
   higher than `-O0` causes test failures with `dynamic_cast`. See
   https://android-review.googlesource.com/#/c/185920
