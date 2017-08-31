Changelog
=========

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

Announcements
-------------

 * The deprecated headers have been removed. [Unified Headers] are now simply
   The Headers.

   For migration tips, see [Unified Headers Migration Notes].

 * GCC is no longer supported. It will not be removed from the NDK just yet, but
   is no longer receiving backports. It cannot be removed until after libc++ has
   become stable enough to be the default, as some parts of gnustl are still
   incompatible with Clang. It will be removed when the other STLs are removed
   in r18.

 * `libc++` is out of beta and is now the preferred STL in the NDK. Starting in
   r17, `libc++` is the default STL for CMake and standalone toolchains. If you
   manually selected a different STL, we strongly encourage you to move to
   `libc++`. For more details, see this blog post. TODO: Create blog post.

 * Support for ARM5 (armeabi) is deprecated. It will no longer build by default
   with ndk-build, but is still buildable if it is explicitly named, and will be
   included by "all" and "all32".

   Both CMake and ndk-build will issue a warning if you target this ABI.

[Unified Headers]: https://android.googlesource.com/platform/ndk/+/ndk-release-r16/docs/UnifiedHeaders.md
[Unified Headers Migration Notes]: https://android.googlesource.com/platform/ndk/+/ndk-release-r16/docs/UnifiedHeadersMigration.md

NDK
===

 * ndk-build and CMake now link libatomic by default. Manually adding `-latomic`
   to your ldflags should no longer be necessary.
 * Clang static analyzer support for ndk-build has been fixed to work with Clang
   as a compiler. See https://github.com/android-ndk/ndk/issues/362.

libc++
======

 * libandroid\_support now contains only APIs needed for supporting libc++ on
   old devices. See https://github.com/android-ndk/ndk/issues/300.

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * gabi++ (and therefore stlport) binaries can segfault when built for armeabi:
   https://github.com/android-ndk/ndk/issues/374.
 * MIPS64 must use the integrated assembler. Clang defaults to using binutils
   rather than the integrated assmebler for this target. ndk-build and cmake
   handle this for you, but make sure to use `-fintegrated-as` for MIPS64 for
   custom build systems. See https://github.com/android-ndk/ndk/issues/399.
