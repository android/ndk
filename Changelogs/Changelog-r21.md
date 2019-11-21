# Changelog

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

## Announcements

 * 32-bit Windows is no longer supported. This does not affect the vast majority
   of users. If you do still need to build NDK apps from 32-bit versions of
   Windows, continue using NDK r20.

   For more information on this change within Android Developer tools, see the
   [blog post] on the topic.

[blog post]: https://android-developers.googleblog.com/2019/06/moving-android-studio-and-android.html

 * [LLD](https://lld.llvm.org/) is now available for testing. AOSP has switched
   to using LLD by default and the NDK will follow (timeline unknown). Test LLD
   in your app by passing `-fuse-ld=lld` when linking. Note that [Issue 843]
   will affect builds using LLD with binutils strip and objcopy as opposed to
   llvm-strip and llvm-objcopy.

 * The Play Store will require 64-bit support when uploading an APK beginning in
   August 2019. Start porting now to avoid surprises when the time comes. For
   more information, see [this blog post](https://android-developers.googleblog.com/2017/12/improving-app-security-and-performance.html).

## Changes

 * Updated Clang and LLD to r365631.
     * [Issue 855]: LLD no longer hangs when using multithreaded linking on
       Windows.
     * [Issue 884]: Clang no longer passes `-faddrsig` by default.
     * [Issue 859]: Clang now default to using Neon for all 32-bit Android Arm
       targets.
         * If your minSdkVersion is 23 or higher, or if you were already
           enabling Neon manually, this change does not affect you.
         * CPUs that do not support this feature are uncommon.
         * If you need to continue supporting these devices you can disable
           Neon explicitly by setting `LOCAL_ARM_NEON := false` in ndk-build or
           passing `-DANDROID_ARM_NEON=false` to CMake.
         * Alternatively, use the Play Console to [blacklist CPUs] without
           Neon to disallow your app from being installed on those devices.
     * [Issue 1004]: Fixed bug with bad line number information when building
       Arm64 with `-O0`.
     * [Issue 1028]: OpenMP is now available in both static and shared variants.
       Note that shared is the default behavior, but in prior NDK releases the
       static library would be used because there was no shared library. To keep
       the old behavior, link with `-static-openmp`.
 * Updated libc++ to r369764.
 * Updated make to 4.2.1.
     * Modified ndk-build to supply `-O` for more readable errors with parallel
       builds.
     * `abspath` now works properly with Windows drive letters, so if you're
       using the [workaround] similar to what's found in the NDK samples, you'll
       need to either drop the workaround or make sure it's only used prior to
       r21.
 * Updated glibc to 2.17.
 * Updated gdb to 8.3.
 * [Issue 885]: For LLD+LLDB compatibility, the NDK build systems now pass
   `-Wl,--build-id=sha1` instead of `-Wl,--build-id` when using LLD. Note that
   the CMake toolchain does not have access to flags set in CMakeLists.txt, so
   using an explicit `-fuse-ld=lld` instead of `ANDROID_LD=lld` will produce
   output that cannot be debugged with Android Studio. Third-party build systems
   need to apply the workaround manually. For more details, see the [Build
   System Maintainers Guide][maintainer_linkers].
 * Fixed ndk-build to use Clang's default C++ standard version (currently C++14)
   when using libc++.
 * Removed the `cxx-stl/system` module from ndk-build. The system STL is still
   available (for now, see [Issue 744]); only an implementation detail has been
   removed. If you were explicitly linking libstdc++ or importing
   `cxx-stl/system`, remove that and ensure that `APP_STL` is set to `system`
   instead.
 * [Issue 976]: ndk-build and the CMake toolchain file now enable
   `_FORTIFY_SOURCE` by default. See [FORTIFY in Android] for more details.
 * Added `NDK_GRADLE_INJECTED_IMPORT_PATH` support to ndk-build. This is to
   support import of dependencies from AAR dependencies. See [Issue 916] for
   more information.
 * Added `NDK_MAJOR`, `NDK_MINOR`, `NDK_BETA`, `NDK_CANARY`, and
   `ndk-major-at-least` APIs to ndk-build. These can be used to check what
   version of the NDK you are using from within ndk-build (CMake already has
   equivalent APIs). The first three are integers, `NDK_CANARY` is either `true`
   or `false`, and `ndk-major-at-least` is a function that takes a single
   integer. For example, to check if your build is being performed with NDK r21
   or newer:

   ```makefile
   # This check works even on pre-r21 NDKs. The function is undefined pre-r21,
   # and calling an undefined function in make returns the empty string, which
   # is not equal to "true", so the else branch will be taken.
   ifeq ($(call ndk-major-at-least,21),true)
       # Using at least NDK r21.
   else
       # Using something earlier than r21.
   endif
   ```

   Note that because this API was not available before r21, it cannot be used to
   determine *which* NDK version earlier than 21 is being used.
 * [Issue 1092]: Fixed hiding of unwinder symbols in outputs of ndk-build and
   CMake. Maintainers of third-party build systems should apply similar fixes
   when using NDK r19 and above to guard against possible future compatibility
   issues.

[FORTIFY in Android]: https://android-developers.googleblog.com/2017/04/fortify-in-android.html
[Issue 1004]: https://github.com/android-ndk/ndk/issues/1004
[Issue 1028]: https://github.com/android/ndk/issues/1028
[Issue 1092]: https://github.com/android/ndk/issues/1092
[Issue 744]: https://github.com/android/ndk/issues/744
[Issue 855]: https://github.com/android-ndk/ndk/issues/855
[Issue 859]: https://github.com/android-ndk/ndk/issues/859
[Issue 884]: https://github.com/android-ndk/ndk/issues/884
[Issue 885]: https://github.com/android-ndk/ndk/issues/885
[Issue 916]: https://github.com/android-ndk/ndk/issues/916
[Issue 976]: https://github.com/android/ndk/issues/976
[blacklist CPUs]: https://support.google.com/googleplay/android-developer/answer/7353455?hl=en
[maintainer_linkers]: https://android.googlesource.com/platform/ndk/+/master/docs/BuildSystemMaintainers.md#Linkers
[workaround]: https://github.com/android/ndk-samples/blob/2c97a9eb5b9b5de233b7ece4dd0d0d28fa4cb4c2/other-builds/ndkbuild/common.mk#L26

## Known Issues

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * [Issue 360]: `thread_local` variables with non-trivial destructors will cause
   segfaults if the containing library is `dlclose`ed on devices running M or
   newer, or devices before M when using a static STL. The simple workaround is
   to not call `dlclose`.
 * [Issue 70838247]: Gold emits broken debug information for AArch64. AArch64
   still uses BFD by default.
 * [Issue 906]: Clang does not pass `-march=armv7-a` to the assembler when using
   `-fno-integrated-as`. This results in the assembler generating ARMv5
   instructions. Note that by default Clang uses the integrated assembler which
   does not have this problem. To workaround this issue, explicitly use
   `-march=armv7-a` when building for 32-bit ARM with the non-integrated
   assembler, or use the integrated assembler. ndk-build and CMake already
   contain these workarounds.
 * [Issue 906]: Clang does not pass `-march=armv7-a` to the assembler when using
   `-fno-integrated-as`. This results in the assembler generating ARMv5
   instructions. Note that by default Clang uses the integrated assembler which
   does not have this problem. To workaround this issue, explicitly use
   `-march=armv7-a` when building for 32-bit ARM with the non-integrated
   assembler, or use the integrated assembler. ndk-build and CMake already
   contain these workarounds.
 * [Issue 988]: Exception handling when using ASan via wrap.sh can crash. To
   workaround this issue when using libc++_shared, ensure that your
   application's libc++_shared.so is in `LD_PRELOAD` in your `wrap.sh` as in the
   following example:

   ```bash
   #!/system/bin/sh
   HERE="$(cd "$(dirname "$0")" && pwd)"
   export ASAN_OPTIONS=log_to_syslog=false,allow_user_segv_handler=1
   ASAN_LIB=$(ls $HERE/libclang_rt.asan-*-android.so)
   if [ -f "$HERE/libc++_shared.so" ]; then
       # Workaround for https://github.com/android-ndk/ndk/issues/988.
       export LD_PRELOAD="$ASAN_LIB $HERE/libc++_shared.so"
   else
       export LD_PRELOAD="$ASAN_LIB"
   fi
   "$@"
    ```

   There is no known workaround for libc++_static.

   Note that because this is a platform bug rather than an NDK bug this
   workaround will be necessary for this use case to work on all devices until
   at least Android R.
 * This version of the NDK is incompatible with the Android Gradle plugin
   version 3.0 or older. If you see an error like
   `No toolchains found in the NDK toolchains folder for ABI with prefix: mips64el-linux-android`,
   update your project file to [use plugin version 3.1 or newer]. You will also
   need to upgrade to Android Studio 3.1 or newer.
 * [Issue 843]: Using LLD with binutils `strip` or `objcopy` breaks RelRO.

[Issue 360]: https://github.com/android-ndk/ndk/issues/360
[Issue 70838247]: https://issuetracker.google.com/70838247
[Issue 843]: https://github.com/android-ndk/ndk/issues/843
[Issue 906]: https://github.com/android-ndk/ndk/issues/906
[Issue 988]: https://github.com/android-ndk/ndk/issues/988
[use plugin version 3.1 or newer]: https://developer.android.com/studio/releases/gradle-plugin#updating-plugin
