# Changelog

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

## Announcements

 * [LLD](https://lld.llvm.org/) is now available for testing. AOSP is in the
   process of switching to using LLD by default and the NDK will follow
   (timeline unknown). Test LLD in your app by passing `-fuse-ld=lld` when
   linking.

 * The Play Store will require 64-bit support when uploading an APK beginning in
   August 2019. Start porting now to avoid surprises when the time comes. For
   more information, see [this blog post](https://android-developers.googleblog.com/2017/12/improving-app-security-and-performance.html).

## r20b

 * Fix for CVE-2019-2212, out of bounds buffer read in `std::random`. See
   https://bugs.chromium.org/p/chromium/issues/detail?id=994957 for more
   information.

## Changes

 * Updated Clang to r346389c.
 * Updated libc++ to r350972.
 * Add Android Q Beta 1 APIs:
     * MIDI (`<amidi/AMidi.h>`).
     * Binder.
     * Extensions to several APIs from previous releases.
 * [Issue 908]: `LOCAL_LDFLAGS` now take precedence over `APP_LDFLAGS`.

[Issue 908]: https://github.com/android-ndk/ndk/issues/908

## Known Issues

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * [Issue 360]: `thread_local` variables with non-trivial destructors will cause
   segfaults if the containing library is `dlclose`ed on devices running M or
   newer, or devices before M when using a static STL. The simple workaround is
   to not call `dlclose`.
 * [Issue 70838247]: Gold emits broken debug information for AArch64. AArch64
   still uses BFD by default.
 * [Issue 855]: LLD may hang on Windows when using multithreaded linking.
   ndk-build will automatically disable multithreaded linking in this situation,
   but CMake users and custom build systems should pass `-Wl,--no-threads` when
   using LLD on Windows. The other linkers and operating systems are unaffected.
 * [Issue 884]: Third-party build systems must pass `-fno-addrsig` to Clang for
   compatibility with binutils. ndk-build, CMake, and standalone toolchains
   handle this automatically.
 * [Issue 888]: lld does not support compressed symbols on Windows. Clang also
   cannot generate compressed symbols on Windows, but this can be a problem when
   using artifacts built from Darwin or Linux.
 * [Issue 906]: Clang does not pass `-march=armv7-a` to the assembler when using
   `-fno-integrated-as`. This results in the assembler generating ARMv5
   instructions. Note that by default Clang uses the integrated assembler which
   does not have this problem. To workaround this issue, explicitly use
   `-march=armv7-a` when building for 32-bit ARM with the non-integrated
   assembler, or use the integrated assembler. ndk-build and CMake already
   contain these workarounds.
 * [Issue 1004]: Clang outputs debug info with bad line number info when
   compiling for ARM64 and `-O0` (no optimizations). Third-party build systems
   can pass `-fno-experimental-isel` to Clang to work around this issue.
 * This version of the NDK is incompatible with the Android Gradle plugin
   version 3.0 or older. If you see an error like
   `No toolchains found in the NDK toolchains folder for ABI with prefix: mips64el-linux-android`,
   update your project file to [use plugin version 3.1 or newer]. You will also
   need to upgrade to Android Studio 3.1 or newer.

[Issue 360]: https://github.com/android-ndk/ndk/issues/360
[Issue 70838247]: https://issuetracker.google.com/70838247
[Issue 855]: https://github.com/android-ndk/ndk/issues/855
[Issue 884]: https://github.com/android-ndk/ndk/issues/884
[Issue 888]: https://github.com/android-ndk/ndk/issues/888
[Issue 906]: https://github.com/android-ndk/ndk/issues/906
[Issue 1004]: https://github.com/android-ndk/ndk/issues/1004
[use plugin version 3.1 or newer]: https://developer.android.com/studio/releases/gradle-plugin#updating-plugin
