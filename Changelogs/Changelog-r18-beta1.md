Changelog
=========

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

Announcements
-------------

 * GCC has been removed.

 * [LLD](https://lld.llvm.org/) is now available for testing. AOSP is in the
   process of switching to using LLD by default and the NDK will follow
   (timeline unknown). Test LLD in your app by passing `-fuse-ld=lld` when
   linking.

 * gnustl, gabi++, and stlport have been removed.

 * Support for ICS (android-14 and android-15) has been removed. Apps using
   executables no longer need to provide both a PIE and non-PIE executable.

 * The Play Store will require 64-bit support when uploading an APK beginning in
   August 2019. Start porting now to avoid surprises when the time comes. For
   more information, see [this blog post](https://android-developers.googleblog.com/2017/12/improving-app-security-and-performance.html).

Changes
-------

 * Updated Clang to build 4751641, based on r328903.
     * With [Issue 573] fixed, -Oz is now the default optimization mode for
       thumb.
 * Updated libc++ to revision r334917.
 * Added support for clang-tidy to ndk-build.
     * Enable application-wide with `APP_CLANG_TIDY := true`, or per-module with
       `LOCAL_CLANG_TIDY := true`.
     * Pass specific clang-tidy flags such as `-checks` with
       `APP_CLANG_TIDY_FLAGS` or `LOCAL_CLANG_TIDY_FLAGS`.
     * As usual, module settings override application settings.
     * By default no flags are passed to clang-tidy, so only the checks enabled
       by default in clang-tidy will be enabled. View the default list with
       `clang-tidy -list-checks`.
     * By default clang-tidy warnings are not errors. This behavior can be
       changed with `-warnings-as-errors=*`.
 * [Issue 490]: ndk-build and CMake now default to using NEON for ARM when
   targeting android-23 (Marshmallow) or newer.
     * If your minSdkVersion is below 23 or if you were already enabling NEON
       manually, this change does not affect you.
     * CPUs that do not support this feature are uncommon and new devices were
       not allowed to ship without it beginning in Marshmallow, but older
       devices that did not support NEON may have been upgraded to Marshmallow.
     * If you need to continue supporting these devices and have a minSdkVersion
       of 23 or higher, you can disable NEON explicitly by setting
       `LOCAL_ARM_NEON := false` in ndk-build or passing
       `-DANDROID_ARM_NEON=false` to CMake.
     * Alternatively, use the Play Console to [blacklist CPUs] without NEON to
       disallow your app from being installed on those devices.

[Issue 490]: https://github.com/android-ndk/ndk/issues/490
[Issue 573]: https://github.com/android-ndk/ndk/issues/573
[blacklist CPUs]: https://support.google.com/googleplay/android-developer/answer/7353455?hl=en
[clang-tidy]: http://clang.llvm.org/extra/clang-tidy/

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * [Issue 360]: `thread_local` variables with non-trivial destructors will cause
   segfaults if the containing library is `dlclose`ed on devices running M or
   newer, or devices before M when using a static STL. The simple workaround is
   to not call `dlclose`.
 * [Issue 70838247]: Gold emits broken debug information for AArch64. AArch64
   still uses BFD by default.
 * This version of the NDK is incompatible with the Android Gradle plugin
   version 3.0 or older. If you see an error like
   `No toolchains found in the NDK toolchains folder for ABI with prefix: mips64el-linux-android`,
   update your project file to [use plugin version 3.1 or newer]. You will also
   need to upgrade to Android Studio 3.1 or newer.

[Issue 360]: https://github.com/android-ndk/ndk/issues/360
[Issue 70838247]: https://issuetracker.google.com/70838247
[use plugin version 3.1 or newer]: https://developer.android.com/studio/releases/gradle-plugin#updating-plugin
