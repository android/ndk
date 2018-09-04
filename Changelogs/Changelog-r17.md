Changelog
=========

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

Announcements
-------------

 * GCC is no longer supported. It will be removed in NDK r18.

 * `libc++` is now the default STL for CMake and standalone toolchains. If you
   manually selected a different STL, we strongly encourage you to move to
   `libc++`. Note that ndk-build still defaults to no STL. For more details, see
   [this blog post](https://android-developers.googleblog.com/2017/09/introducing-android-native-development.html).

 * gnustl and stlport are deprecated and will be removed in NDK r18.

 * Support for ARMv5 (armeabi), MIPS, and MIPS64 has been removed. Attempting to
   build any of these ABIs will result in an error.

 * Support for ICS (android-14 and android-15) will be removed from r18.

 * The Play Store will require 64-bit support when uploading an APK beginning in
   August 2019. Start porting now to avoid surprises when the time comes. For
   more information, see [this blog post](https://android-developers.googleblog.com/2017/12/improving-app-security-and-performance.html).

r17c
----

 * [Issue 112611166]: Fixed "min" value in meta/platforms.json.

[Issue 112611166]: https://issuetracker.google.com/112611166

r17b
----

 * [Issue 480]: Fixed libc++ header issues with `_FILE_OFFSET_BITS=64`.
 * [Issue 692]: Fixed ASAN for standalone toolchains with static STLs.
 * [Issue 697]: Pulled documentation updates for simpleperf.
 * [Issue 702]: Updated sysroot to fix missing libc symbols.
 * [Issue 704]: Updated shaderc/vulkan-validation-layers sources.
 * [Issue 708]: Fixed introduced API tag for
   `pthread_cond_timedwait_monotonic_nmp` to make it available on ICS.

[Issue 480]: https://github.com/android-ndk/ndk/issues/480
[Issue 692]: https://github.com/android-ndk/ndk/issues/692
[Issue 697]: https://github.com/android-ndk/ndk/issues/697
[Issue 702]: https://github.com/android-ndk/ndk/issues/702
[Issue 704]: https://github.com/android-ndk/ndk/issues/704
[Issue 708]: https://github.com/android-ndk/ndk/issues/708

Changes
-------

 * Updated Clang to build 4691093, based on r316199.
     * [Issue 313]: LTO now works on Windows.
     * [Issue 527]: [Undefined Behavior Sanitizer] now works for NDK binaries.
     * [Issue 593]: Clang now automatically passes `--eh-frame-hdr` to the
       linker even for static executables, fixing exception handling.
 * Updated gtest to upstream revision 0fe96607d85cf3a25ac40da369db62bbee2939a5.
 * `libandroid_support` is no longer used when your NDK API level is greater
   than or equal to 21 (Lollipop). Build system maintainers: be sure to update
   your build systems to account for this.
 * The platform static libraries (libc.a, libm.a, etc.) have been updated.
     * All NDK platforms now contain a modern version of these static libraries.
       Previously they were all Gingerbread (perhaps even older) or Lollipop.
     * Prior NDKs could not use the static libraries with a modern NDK API level
       because of symbol collisions between libc.a and libandroid_support. This
       has been solved by removing libandroid_support for modern API levels. A
       side effect of this is that you must now target at least android-21 to
       use the static libraries, but these binaries will still work on older
       devices.
 * Fixed parsing of the NDK revision in CMake. NDK version information is now
   available in the following CMake variables:
     * `ANDROID_NDK_REVISION`: The full string in the source.properties file.
     * `ANDROID_NDK_MAJOR`: The major revision of the NDK. For example: the 16
       in r16b.
     * `ANDROID_NDK_MINOR`: The minor revision of the NDK. For example: the b
       (represented as 1) in r16b.
     * `ANDROID_NDK_BUILD`: The build number of the NDK. This is 0 in the case
       of a local development build.
     * `ANDROID_NDK_BETA`: The beta version of the NDK. This is 0 for a stable
       release.
 * Added support for `APP_WRAP_SH` to ndk-build.
     * This variable points to a shell script (relative to your Android.mk) that
       will be installed as a [wrap.sh] file in your APK.
     * Available in both an ABI-generic form (`APP_WRAP_SH`), which will install
       a single script for every ABI, and an ABI-specific form
       (`APP_WRAP_SH_arm64-v8a`, etc) to allow for per-ABI customization of the
       wrap.sh script.
 * [Issue 540]: ndk-build now installs sanitizer runtime libraries to your out
   directory for inclusion in your APK. Coupled with [wrap.sh], this removes the
   requirement of rooting your device to use sanitizers.
 * When using ASAN, ndk-build will install a wrap.sh file to set up ASAN for
   your app if you have not specified your own wrap.sh. If you have specified
   your own wrap.sh, you can add ASAN support to it as described
   [here](https://github.com/google/sanitizers/wiki/AddressSanitizerOnAndroidO).
 * [Issue 614]: ndk-build and CMake no longer pass `-fno-integrated-as` for
   arm32 by default.  If your project contains hand written assembly, you may
   need to pass this flag.
 * ion.h is no longer a part of the NDK. This file defined an API that is not
   consistent across devices.

[Undefined Behavior Sanitizer]: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
[wrap.sh]: https://developer.android.com/ndk/guides/wrap-script.html
[Issue 313]: https://github.com/android-ndk/ndk/issues/313
[Issue 527]: https://github.com/android-ndk/ndk/issues/527
[Issue 540]: https://github.com/android-ndk/ndk/issues/540
[Issue 593]: https://github.com/android-ndk/ndk/issues/593
[Issue 614]: https://github.com/android-ndk/ndk/issues/614

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * [Issue 360]: `thread_local` variables with non-trivial destructors will cause
   segfaults if the containing library is `dlclose`ed on devices running M or
   newer, or devices before M when using a static STL. The simple workaround is
   to not call `dlclose`.
 * [Issue 70838247]: Gold emits broken debug information for AArch64. AArch64
   still uses BFD by default.

[Issue 360]: https://github.com/android-ndk/ndk/issues/360
[Issue 70838247]: https://issuetracker.google.com/70838247
