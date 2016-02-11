Changelog
=========

We've moved our bug tracker to GitHub: https://github.com/android-ndk/ndk/issues

Clang
-----

 * **PSA: Everyone should be switching to Clang.**
 * Clang has been updated to 3.8svn (r243773, build 2481030).
     * Note that this is now a nearly pure upstream clang.
     * Also note that Clang packaged in the Windows 64 NDK is actually 32-bit.
 * Support for emulated TLS.
     * `__thread` is now supported by the compiler by emulating ELF TLS with
       pthread thread-specific data.
     * C++11 `thread_local` will work in some cases, but will not work for data
       with non-trivial destructors except when running on Marshmallow
       (android-23) or newer because those cases require support from libc.
     * Does not yet work with Aarch64 when TLS variables are accessed from a
       shared library.

GCC
---

 * **GCC in the NDK is now deprecated.**
     * Time to start using Clang if you haven’t already. If you have problems
       with Clang, please file bugs!
     * The NDK will not be upgrading to 5.x, nor will we be accepting
       non-critical backports.
     * Maintenance for miscompiles and internal compiler errors in 4.9 will be
       handled on a case by case basis.
 * GCC 4.8 has been removed. All targets now use GCC 4.9.
 * Synchronized with `google/gcc-4_9` to r224707 (from r214835).

NDK
---

 * The samples are no longer included in the NDK. They are instead available on
   [GitHub].
 * The documentation is no longer included in the NDK. It is instead available
   on the [Android Developer website].
 * Make ARM standalone toolchains default to arm7.
     * The old behavior can be restored by passing
       `-target armv5te-linux-androideabi`.
 * Use `-isystem` for platform includes.
     * Warnings caused by bionic will no longer break app builds.
 * Fixed segfault when throwing exceptions via gabi++ (see
   http://b.android.com/179410).
 * Change libc++’s inline namespace to `std::__ndk1` to prevent ODR issues with
   platform libc++.
 * Support for mips64r2 has been partially dropped. The rest will be dropped in
   the future.
 * All libc++ libraries are now built with libc++abi.
 * Bump default `APP_PLATFORM` to Gingerbread.
     * Expect support for Froyo and older to be dropped in a future release.
 * Updated gabi++ `_Unwind_Exception` struct for 64 bits.
 * cpufeatures: Detect SSE4.1 and SSE4.2 as well.
 * cpufeatures: Detect cpu features on x86\_64 as well.
 * Update libc++abi to upstream r231075.
 * Added native tracing API to android-23.
 * Added native multinetwork API to android-23.
 * `byteswap.h`, `endian.h`, `sys/procfs.h`, `sys/ucontext.h`, `sys/user.h`, and
   `uchar.h` have all been updated from ToT Bionic.
 * `sys/cdefs.h` has been synchronized across all API levels.
 * Support for `_WCHAR_IS_8BIT` has been removed.
 * Fixed `fegetenv` and `fesetenv` for arm
 * Fix end pointer size/alignment of `crtend_*` for mips64 and x86\_64
 * Removed sed.
 * Removed mclinker.
 * Removed Perl.
 * Removed symbols which are not exported by the current platform libc/m/dl from
   all versions of NDK libc/m/dl
 * libc/m/dl provide versioned symbols starting with v21

Binutils
--------

 * Unified binutils source between Android and ChromiumOS.
 * For full details see https://android-review.googlesource.com/#/c/182865/.
 * Gold for aarch64 should now be much more reliable. Use `-fuse-ld=gold` at
   link time to use gold instead of bfd. The default will likely switch in the
   next release.
 * Good linking time improvement for huge binaries for Gold ARM backend (up to
   50% linking time reduction for debuggable Chrome Browser).
 * New option: `--pic-veneer`.
 * The 32-bit Windows package no longer contains ld.gold. It is available in
   the 64-bit package.
     * Current gold no longer builds when targeting 32-bit Windows (causes
       internal compiler failures in mingw).

GDB
---

 * gdb has been updated to version 7.10.
 * ndk-gdb has been removed in favor of ndk-gdb.py.
 * ndk-gdb.py has been significantly rewritten.
   * Performance should be somewhat better.
   * Error messages have been significantly improved.
   * Relative project paths should always work now.
   * Ctrl-C no longer kills the backgrounded gdbserver.
   * Improve Windows support.

Yasm
----

 * Yasm has been updated to version 1.3.0.

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * x86 ASAN does not currently work. See discussion on
   https://android-review.googlesource.com/#/c/186276/
 * The combination of Clang, x86, `stlport_static`, and optimization levels
   higher than `-O0` causes test failures with `dynamic_cast`. See
   https://android-review.googlesource.com/#/c/185920
 * Exception handling will often fail when using `c++_shared` on ARM32. The root
   cause is incompatibility between the LLVM unwinder used by libc++abi for
   ARM32 and libgcc. This is not a regression from r10e.

[GitHub]: https://github.com/googlesamples/android-ndk
[Android Developer website]: http://developer.android.com/ndk/index.html
