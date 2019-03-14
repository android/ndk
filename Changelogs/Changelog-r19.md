Changelog
=========

Report issues to [GitHub].

For Android Studio issues, follow the docs on the [Android Studio site].

[GitHub]: https://github.com/android-ndk/ndk/issues
[Android Studio site]: http://tools.android.com/filing-bugs

Announcements
-------------

 * Developers should begin testing their apps with [LLD](https://lld.llvm.org/).
   AOSP has switched to using LLD by default and the NDK will use it by default
   in the next release. BFD and Gold will be removed once LLD has been through a
   release cycle with no major unresolved issues (estimated r21). Test LLD in
   your app by passing `-fuse-ld=lld` when linking.

   Note: lld does not currently support compressed symbols on Windows. See
   [Issue 888]. Clang also cannot generate compressed symbols on Windows, but
   this can be a problem when using artifacts built from Darwin or Linux.

 * The Play Store will require 64-bit support when uploading an APK beginning in
   August 2019. Start porting now to avoid surprises when the time comes. For
   more information, see [this blog post](https://android-developers.googleblog.com/2017/12/improving-app-security-and-performance.html).

 * [Issue 780]: [Standalone toolchains] are now unnecessary. Clang, binutils,
   the sysroot, and other toolchain pieces are now all installed to
   `$NDK/toolchains/llvm/prebuilt/<host-tag>` and Clang will automatically find
   them. Instead of creating a standalone toolchain for API 26 ARM, instead
   invoke the compiler directly from the NDK:

       $ $NDK/toolchains/llvm/prebuilt/<host-tag>/bin/armv7a-linux-androideabi26-clang++ src.cpp

   For r19 the toolchain is also installed to the old path to give build systems
   a chance to adapt to the new layout. The old paths will be removed in r20.

   The `make_standalone_toolchain.py` script will not be removed. It is now
   unnecessary and will emit a warning with the above information, but the
   script will remain to preserve existing workflows.

   If you're using ndk-build, CMake, or a standalone toolchain, there should be
   no change to your workflow. This change is meaningful for maintainers of
   third-party build systems, who should now be able to delete some
   Android-specific code. For more information, see the [Build System
   Maintainers] guide.

 * ndk-depends has been removed. We believe that [ReLinker] is a better
   solution to native library loading issues on old Android versions.

 * [Issue 862]: The GCC wrapper scripts which redirected to Clang have been
   removed, as they are not functional enough to be drop in replacements.

[Build System Maintainers]: https://android.googlesource.com/platform/ndk/+/master/docs/BuildSystemMaintainers.md
[Issue 780]: https://github.com/android-ndk/ndk/issues/780
[Issue 862]: https://github.com/android-ndk/ndk/issues/862
[ReLinker]: https://github.com/KeepSafe/ReLinker
[Standalone toolchains]: https://developer.android.com/ndk/guides/standalone_toolchain

r19c
----

 * [Issue 912]: Prevent the CMake toolchain file from clobbering a user
   specified `CMAKE_FIND_ROOT_PATH`.
 * [Issue 920]: Fix clang wrapper scripts on Windows.

[Issue 912]: https://github.com/android-ndk/ndk/issues/912
[Issue 920]: https://github.com/android-ndk/ndk/issues/920

r19b
----

 * [Issue 855]: ndk-build automatically disables multithreaded linking for LLD
   on Windows, where it may hang. It is not possible for the NDK to detect this
   situation for CMake, so CMake users and custom build systems must pass
   `-Wl,--no-threads` when linking with LLD on Windows.
 * [Issue 849]: Fixed unused command line argument warning when using standalone
   toolchains to compile C code.
 * [Issue 890]: Fixed `CMAKE_FIND_ROOT_PATH`. CMake projects will no longer
   search the host's sysroot for headers and libraries.
 * [Issue 906]: Explicitly set `-march=armv7-a` for 32-bit ARM to workaround
   Clang not setting that flag automatically when using `-fno-integrated-as`.
   This fix only affects ndk-build and CMake. Standalone toolchains and custom
   build systems will need to apply this fix themselves.
 * [Issue 907]: Fixed `find_path` for NDK headers in CMake.

[Issue 849]: https://github.com/android-ndk/ndk/issues/849
[Issue 890]: https://github.com/android-ndk/ndk/issues/890
[Issue 907]: https://github.com/android-ndk/ndk/issues/907

Changes
-------

 * Updated Clang to r339409.
     * C++ compilation now defaults to C++14.
 * [Issue 780]: A complete NDK toolchain is now installed to the Clang
   directory. See the announcements section for more information.
 * ndk-build no longer removes artifacts from `NDK_LIBS_OUT` for ABIs not
   present in `APP_ABI`. This enables workflows like the following:

   ```bash
   for abi in armeabi-v7a arm64-v8a x86 x86_64; do
       ndk-build APP_ABI=$abi
   done
   ```

   Prior to this change, the above workflow would remove the previously built
   ABI's artifacts on each successive build, resulting in only x86_64 being
   present at the end of the loop.
 * ndk-stack has been rewritten in Python.
 * [Issue 776]: To better support LLD, ndk-build and CMake no longer pass
   `-Wl,--fix-cortex-a8` by default.
     * CPUs that require this fix are uncommon in the NDK's supported API range
       (16+).
     * If you need to continue supporting these devices, add
       `-Wl,--fix-cortex-a8` to your `APP_LDFLAGS` or `CMAKE_C_FLAGS`, but note
       that LLD will not be adding support for this workaround.
     * Alternatively, use the Play Console to [blacklist] Cortex-A8 CPUs to
       disallow your app from being installed on those devices.
 * [Issue 798]: The ndk-build and CMake options to disable RelRO and noexecstack
   are now ignored. All code is built with RelRO and non-executable stacks.
 * [Issue 294]: All code is now built with a subset of [compiler-rt] to provide
   a complete set of compiler built-ins for Clang.

[Issue 294]: https://github.com/android-ndk/ndk/issues/294
[Issue 776]: https://github.com/android-ndk/ndk/issues/776
[Issue 798]: https://github.com/android-ndk/ndk/issues/798
[blacklist]: https://support.google.com/googleplay/android-developer/answer/7353455?hl=en
[compiler-rt]: https://compiler-rt.llvm.org/

Known Issues
------------

 * This is not intended to be a comprehensive list of all outstanding bugs.
 * [Issue 888]: lld does not support compressed symbols on Windows. Clang also
   cannot generate compressed symbols on Windows, but this can be a problem when
   using artifacts built from Darwin or Linux.
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
 * [Issue 906]: Clang does not pass `-march=armv7-a` to the assembler when using
   `-fno-integrated-as`. This results in the assembler generating ARMv5
   instructions. Note that by default Clang uses the integrated assembler which
   does not have this problem. To workaround this issue, explicitly use
   `-march=armv7-a` when building for 32-bit ARM with the non-integrated
   assembler, or use the integrated assembler. ndk-build and CMake already
   contain these workarounds.
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
[use plugin version 3.1 or newer]: https://developer.android.com/studio/releases/gradle-plugin#updating-plugin
