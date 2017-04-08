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

 * GCC is no longer supported. It will not be removed from the NDK just yet, but
   is no longer receiving backports. It cannot be removed until after libc++ has
   become stable enough to be the default, as some parts of gnustl are still
   incompatible with Clang. It will likely be removed after that point.

 * Gingerbread (android-9) is no longer supported. The minimum API level target
   in the NDK is now Ice Cream Sandwich (android-14). If your `APP_PLATFORM` is
   set lower than android-14, android-14 will be used instead.

[Unified Headers]: https://android.googlesource.com/platform/ndk/+/master/docs/UnifiedHeaders.md

NDK
===

 * `awk` is no longer in the NDK. We've replaced all uses of awk with Python.

libc++
======

 * We've begun slimming down and improving `libandroid_support` to fix libc++
   reliability issues. https://github.com/android-ndk/ndk/issues/300.

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * Gradle does not yet support unified headers.
 * Clang does not yet support OpenMP:
   https://github.com/android-ndk/ndk/issues/9.
