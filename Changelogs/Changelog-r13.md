Changelog
=========

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

Announcements
-------------

* GCC is no longer supported. It will not be removed from the NDK just yet, but
  is no longer receiving backports. It cannot be removed until after libc++ has
  become stable enough to be the default, as some parts of gnustl are still
  incompatible with Clang. It will likely be removed after that point.

NDK
---

 * `NDK_TOOLCHAIN_VERSION` now defaults to Clang.
 * libc++ has been updated to r263688.
     * We've reset to a (nearly) clean upstream. This should remove a number of
       bugs, but we still need to clean up `libandroid_support` before we will
       recommend it as the default.
 * `make-standalone-toolchain.sh` is now simply a wrapper around the Python
   version of the tool. There are a few behavioral differences. See
   https://android-review.googlesource.com/#/c/245453/
 * Some libraries for unsupported ABIs have been removed (mips64r2, mips32r6,
   mips32r2, and x32). There might still be some stragglers.
 * Issues with `crtbegin_static.o` that resulted in missing `atexit` at link
   time when building a static executable for ARM android-21+ have been
   resolved: https://github.com/android-ndk/ndk/issues/132
 * Added CMake toolchain file in build/cmake/android.toolchain.cmake.

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * Standlone toolchains using libc++ and GCC do not work. This seems to be a bug
   in GCC. See the following commit message for more details:
   https://android-review.googlesource.com/#/c/247498
 * x86 ASAN still doesn't work. See discussion on
   https://android-review.googlesource.com/#/c/186276/
 * Exception unwinding with `c++_shared` still does not work for ARM on
   Gingerbread or Ice Cream Sandwich.
 * Bionic headers and libraries for Marshmallow and N are not yet exposed
   despite the presence of android-24. Those platforms are still the Lollipop
   headers and libraries (not a regression from r11).
 * RenderScript tools are not present (not a regression from r11):
   https://github.com/android-ndk/ndk/issues/7.
