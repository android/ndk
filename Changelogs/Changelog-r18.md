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

r18b
----

 * [Issue 799]: Fixed build performance regression in ndk-build caused by
   compile_commands.json logic.
 * [Issue 803]: The "GCC" scripts in standalone toolchains now point to the
   correct Clang.
 * [Issue 805]: The "GCC" wrappers for Clang now use `-gcc-toolchain`.
 * [Issue 815]: ndk-build now builds with `-fstack-protector-strong` again.

[Issue 799]: https://github.com/android-ndk/ndk/issues/799
[Issue 803]: https://github.com/android-ndk/ndk/issues/803
[Issue 805]: https://github.com/android-ndk/ndk/issues/805
[Issue 815]: https://github.com/android-ndk/ndk/issues/805

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
 * ndk-build can now generate a [JSON Compilation Database].
     * Generate with either `ndk-build compile_commands.json` (does not build)
       or `ndk-build GEN_COMPILE_COMMANDS_DB=true` (builds and generates
       database).
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
 * Added `APP_STRIP_MODE` and `LOCAL_STRIP_MODE` to ndk-build.
     * Allows the user to specify the strip mode used for their modules.
     * The option is passed directly to the strip command. See the [strip
       documentation](https://sourceware.org/binutils/docs/binutils/strip.html)
       for details.
     * If set to "none", strip will not be run.
     * Defaults to "--strip-unneeded". This is the same behavior as previous
       NDKs.
     * `LOCAL_STRIP_MODE` always overrides `APP_STRIP_MODE` when set.
 * [Issue 749]: The libc++_shared.so in the NDK is no longer stripped of debug
   info. Debugging libc++ is now possible. Gradle will still strip the library
   before packaging it in an APK.

[Issue 490]: https://github.com/android-ndk/ndk/issues/490
[Issue 573]: https://github.com/android-ndk/ndk/issues/573
[Issue 749]: https://github.com/android-ndk/ndk/issues/749
[blacklist CPUs]: https://support.google.com/googleplay/android-developer/answer/7353455?hl=en
[clang-tidy]: http://clang.llvm.org/extra/clang-tidy/
[JSON Compilation Database]: https://clang.llvm.org/docs/JSONCompilationDatabase.html

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
