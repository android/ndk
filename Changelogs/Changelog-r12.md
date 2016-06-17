Changelog
=========

Report issues to [GitHub].

[GitHub]: https://github.com/android-ndk/ndk/issues

Announcements
-------------

 * `ndk-build` will default to using Clang in r13. GCC will be removed in a
   later release.
 * `make-standalone-toolchain.sh` will be removed in r13. Make sure
   `make_standalone_toolchain.py` suits your needs.

r12b
----

 * `ndk-gdb.py` has been fixed: https://github.com/android-ndk/ndk/issues/118
 * NdkCameraMetadataTags.h has been updated to no longer contain the invalid
   enum value.
 * A bug in `ndk-build` that resulting in spurious warnings for static libraries
   using libc++ has been fixed:
   https://android-review.googlesource.com/#/c/238146/
 * The OpenSLES headers have been updated for android-24.

NDK
---
 * Removed support for the armeabi-v7a-hard ABI. See the explanation in the
   [documentation](docs/HardFloatAbi.md).
 * Removed all sysroots for pre-GB platform levels. We dropped support for them
   in r11, but neglected to actually remove them.
 * Exception handling when using `c++_shared` on ARM32 now mostly works (see
   [Known Issues](#known-issues)). The unwinder will now be linked into each
   linked object rather than into libc++ itself.
 * Default compiler flags have been pruned:
   https://github.com/android-ndk/ndk/issues/27.
     * Full changes here: https://android-review.googlesource.com/#/c/207721/5.
 * Added a Python implementation of standalone toolchains:
   `build/tools/make_standalone_toolchain.py`.
     * Windows users: you no longer need Cygwin to use this feature.
     * We'll be removing the bash flavor in r13, so test the new one now.
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
 * Unreachable ARM (non-thumb) STL libraries have been removed.
 * Added Vulkan support to android-24.
 * Added Choreographer API to android-24.
 * Added `libcamera2` APIs for devices with
   `INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED` or above (see [Camera
   Characteristics]).

[Camera Characteristics]: https://developer.android.com/reference/android/hardware/camera2/CameraCharacteristics.html#INFO_SUPPORTED_HARDWARE_LEVEL

Clang
-----

 * Clang has been updated to 3.8svn (r256229, build 2812033).
     * Note that Clang packaged in the Windows 64-bit NDK is actually 32-bit.
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
 * Some bugfixes for `ndk-gdb.py`.

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * x86 ASAN still does not work. See discussion on
   https://android-review.googlesource.com/#/c/186276/
 * Exception unwinding with `c++_shared` still does not work for ARM on
   Gingerbread or Ice Cream Sandwich.
 * Bionic headers and libraries for Marshmallow and N are not yet exposed
   despite the presence of android-24. Those platforms are still the Lollipop
   headers and libraries (not a regression from r11).
 * RenderScript tools are not present (not a regression from r11):
   https://github.com/android-ndk/ndk/issues/7.
