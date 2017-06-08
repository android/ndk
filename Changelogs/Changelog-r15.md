Changelog
=========

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

Announcements
-------------

 * [Unified Headers] are now enabled by default.

   **Note**: The deprecated headers will be removed in a future release, most
   likely r16. If they do not work for you, file bugs now.

   For migration tips, see [Unified Headers Migration Notes].

 * GCC is no longer supported. It will not be removed from the NDK just yet, but
   is no longer receiving backports. It cannot be removed until after libc++ has
   become stable enough to be the default, as some parts of gnustl are still
   incompatible with Clang. It will likely be removed after that point.

 * Gingerbread (android-9) is no longer supported. The minimum API level target
   in the NDK is now Ice Cream Sandwich (android-14). If your `APP_PLATFORM` is
   set lower than android-14, android-14 will be used instead.

 * The CMake toolchain file now supports building assembly code written in YASM
   to run on x86 and x86-64 architectures. To learn more, see [YASM in CMake].

[Unified Headers]: https://android.googlesource.com/platform/ndk/+/ndk-r15-release/docs/UnifiedHeaders.md
[Unified Headers Migration Notes]: https://android.googlesource.com/platform/ndk/+/ndk-r15-release/docs/UnifiedHeadersMigration.md
[YASM in CMake]: https://android-dot-devsite.googleplex.com/ndk/guides/cmake.html#yasm-cmake

APIs
====

 * Added native APIs for Android O. To learn more about these APIs, see the
   [Native APIs overview].
    * [AAudio API]
    * [Hardware Buffer API]
    * [Shared Memory API]

[Native APIs overview]: https://developer.android.com/ndk/guides/stable_apis.html#a26
[AAudio API]: https://developer.android.com/ndk/reference/a_audio_8h.html
[Hardware Buffer API]: https://developer.android.com/ndk/reference/hardware__buffer_8h.html
[Shared Memory API]: https://developer.android.com/ndk/reference/sharedmem_8h.html

NDK
===

 * `awk` is no longer in the NDK. We've replaced all uses of awk with Python.

Clang
=====

 * Clang has been updated to build 4053586. This is built from Clang 5.0 SVN at
   r300080.
 * Clang now supports OpenMP (except on MIPS/MIPS64):
   https://github.com/android-ndk/ndk/issues/9.

libc++
======

 * We've begun slimming down and improving `libandroid_support` to fix libc++
   reliability issues. https://github.com/android-ndk/ndk/issues/300.

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * gabi++ (and therefore stlport) binaries can segfault when built for armeabi:
   https://github.com/android-ndk/ndk/issues/374.
 * `ndk-gdb` doesn't work on the Samsung Galaxy S8.
   https://android-review.googlesource.com/408522/ fixes this and will be in a
   future release. There is no workaround (but debugging in Android Studio
   should work).
 * MIPS64 must use the integrated assembler. Clang defaults to using binutils
   rather than the integrated assmebler for this target. ndk-build and cmake
   handle this for you, but make sure to use `-fintegrated-as` for MIPS64 for
   custom build systems. See https://github.com/android-ndk/ndk/issues/399.
