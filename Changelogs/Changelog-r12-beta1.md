Changelog
=========

Report issues to [GitHub].

[GitHub]: https://github.com/android-ndk/ndk/issues

NDK
---
 * Removed all sysroots for pre-GB platform levels. We dropped support for them
   in r11, but neglected to actually remove them.
 * Exception handling when using `c++_shared` on ARM32 now mostly works (see
   [Known Issues](#known-issues)). The unwinder will now be linked into each
   linked object rather than into libc++ itself.
 * Default compiler flags have been pruned:
   https://github.com/android-ndk/ndk/issues/27.
     * Full changes here: https://android-review.googlesource.com/#/c/207721/5.
 * `-fno-limit-debug-info` has been enabled by default for Clang debug builds.
   This should improve debugability with LLDB.
 * `--build-id` is now enabled by default.
     * This will be shown in native crash reports so you can easily identify
       which version of your code was running.
 * `NDK_USE_CYGPATH` should no longer cause problems with libgcc:
   http://b.android.com/195486.
 * `-Wl,--warn-shared-textrel` and`-Wl,--fatal-warnings` are now enabled by
   default. If you have shared text relocations, your app will not load on
   Marshmallow or later (and have never been allowed for 64-bit apps).
 * Precompiled headers should work better:
   https://github.com/android-ndk/ndk/issues/14 and
   https://github.com/android-ndk/ndk/issues/16.

Clang
-----

 * Clang has been updated to 3.8svn (r256229, build 2690385).
     * Note that Clang packaged in the Windows 64 NDK is actually 32-bit.
 * `__thread` should work for real this time.

GCC
---

 * Synchronized with the ChromeOS GCC @ `google/gcc-4_9` r227810.
 * Backported coverage sanitizer patch from ToT (r231296).
 * Fixed libatomic to not use ifuncs:
   https://github.com/android-ndk/ndk/issues/31.

Binutils
--------

 * "Erratum 843419 found and fixed" info messages are silenced.
 * Introduced option '--long-plt' to fix internal linker error when linking huge
   arm32 binaries.
 * Fixed wrong run time stubs for AArch64. This was causing jump addresses to be
   calculated incorrectly for very large DSOs.
 * Introduced default option '--no-apply-dynamic' to work around a dynamic
   linker bug for earlier Android releases.
 * NDK r11 KI for `dynamic_cast` not working with Clang, x86, `stlport_static`
   and optimization has been fixed.

GDB
---

 * Updated to GDB 7.11: https://www.gnu.org/software/gdb/news/.

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * x86 ASAN still does work. See discussion on
   https://android-review.googlesource.com/#/c/186276/
 * Exception unwinding with `c++_shared` still does not work for ARM on
   Gingerbread or Ice Cream Sandwich.
 * Bionic headers and libraries for Marshmallow and N are not yet exposed
   despite the presence of android-24. Those platforms are still the Lollipop
   headers and libraries (not a regression from r11).
 * RenderScript tools are not present (not a regression from r11):
   https://github.com/android-ndk/ndk/issues/7.
 * r12 will most likely not work with Android Studio/Gradle right away. The
   Gradle plugin will need to be updated to match the build changes we made in
   `ndk-build`.
 * We've regressed on http://b.android.com/41770.
