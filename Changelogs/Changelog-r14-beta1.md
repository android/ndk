Changelog
=========

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

Announcements
-------------

 * The platform headers have been synchronized with the Android platform. This
   means that the headers are now up to date, accurate, and header only bug
   fixes will affect all API levels. Prior to this, the M and N headers were
   actually the L headers, all the headers did not actually match the platform
   level they were supposedely for (declared functions that didn't exist, didn't
   declare ones that did), and many of the old API levels had missing or
   incorrect constants that were in newer API levels.

   Since these are in some cases radically different from those we've previously
   shipped, these are not enabled by default. See [Unified Headers] for details
   on using the updated headers.

   **Note**: The legacy headers will be removed in a future release. It is
   likely that r15 will default to using these while still being optional. If
   all goes well, the legacy headers will be removed in r16, so please test them
   now.

 * GCC is no longer supported. It will not be removed from the NDK just yet, but
   is no longer receiving backports. It cannot be removed until after libc++ has
   become stable enough to be the default, as some parts of gnustl are still
   incompatible with Clang. It will likely be removed after that point.

[Unified Headers]: https://android.googlesource.com/platform/ndk/+/master/docs/UnifiedHeaders.md

ndk-build
---------

 * Module builds will now fail if they have any missing dependencies. To revert
   to the old behavior, set `APP_ALLOW_MISSING_DEPS=false`. See
   https://github.com/android-ndk/ndk/issues/208.

CMake
-----

 * RTTI and exceptions are now on by default. This was done for improved
   compatibility with existing CMake projects. See
   https://github.com/android-ndk/ndk/issues/212.

Clang/LLVM
----------

 * The x86 ASAN issues noted since r11 appear to have been emulator specific.
   Up to date emulators no longer have this issue.

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * Gradle does not yet support unified headers.
 * RenderScript tools are not present (not a regression from r11):
   https://github.com/android-ndk/ndk/issues/7.

Won't Fix
---------

These issues will not be fixed. They affect only GCC, which is deprecated.

 * Standlone toolchains using libc++ and GCC do not work. This seems to be a bug
   in GCC. See the following commit message for more details:
   https://android-review.googlesource.com/#/c/247498
 * Standalone toolchains using GCC do not work out of the box with unified
   headers. They can be made to work by passing `-D__ANDROID_API__=21`
   (replacing 21 with the same API level you passed to
   `make_standalone_toolchain.py`) when compiling.
