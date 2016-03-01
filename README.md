Android Native Development Kit (NDK)
====================================

The NDK allows Android application developers to include
native code in their Android application packages, compiled as JNI shared
libraries.

Other Resources
---------------

**Note:** This document is for developers _of_ the NDK, not developers
that use the NDK.

This doc gives a high level overview of the NDK's build, packaging, and test
process. For other use cases, or more in depth documentation, refer to the
following sources:

 * User documentation is available on the [Android Developer website].
 * Adding a new NDK API or platform version? Check [Generating Sysroots].
 * Discussions related to the Android NDK happen on the [android-ndk Google
   Group].
 * File bugs against the NDK at https://github.com/android-ndk/ndk/issues.

[Android Developer website]: https://developer.android.com/ndk/index.html
[android-ndk Google Group]: http://groups.google.com/group/android-ndk
[Generating Sysroots]: docs/GeneratingSysroots.md

Building the NDK
================

Both Linux and Windows host binaries are built on Linux machines. Windows host
binaries are built via MinGW cross compiler. Systems without a working MinGW
compiler can use `build/tools/build-mingw64-toolchain.sh` to generate their own
and be added to the `PATH` for build scripts to discover.

Building binaries for Mac OS X requires at least 10.8.

Target headers and binaries are built on Linux.

Components
----------

The NDK consists of three parts: host binaries, target prebuilts, and others
(build system, docs, samples, tests).

### Host Binaries

* `toolchains/` contains GCC and Clang toolchains.
    * `$TOOLCHAIN/config.mk` contains ARCH and ABIS this toolchain can handle.
    * `$TOOLCHAIN/setup.mk` contains toolchain-specific default CFLAGS/LDFLAGS
      when this toolchain is used.
* `prebuilt/$HOST_TAG` contains build dependencies and additional tools.
    * make, awk, python, yasm, and for Windows: cmp.exe and echo.exe
    * `ndk-depends`, `ndk-stack` and `ndk-gdb` can also be found here.

### Target Headers and Binaries

* `platforms/android-$VERSION/arch-$ARCH_NAME/` contains headers and libraries
  for each API level.
    * The build system sets `--sysroot` to one of these directories based on
      user-specified `APP_ABI` and `APP_PLATFORM`.
* `sources/cxx-stl/$STL` contains the headers and libraries for the various C++
  STLs.
* `prebuilt/android-$ARCH/gdbserver` contains gdbserver.

### Others

* `build/` contains the ndk-build system and scripts to rebuild NDK.
* `sources/android` and `sources/third_party` contain modules that can be used
  in apps (cpufeatures, native\_app\_glue, etc) via `$(call import-module,
  $MODULE)`
* `tests/`

Prerequisites
-------------

* [AOSP NDK Repository](http://source.android.com/source/downloading.html)
    * Check out the branch `master-ndk`

        ```bash
        repo init -u https://android.googlesource.com/platform/manifest \
            -b master-ndk

        # Googlers, use
        repo init -u \
            persistent-https://android.git.corp.google.com/platform/manifest \
            -b master-ndk
        ```

* Additional Linux Dependencies (available from apt):
    * bison
    * flex
    * libtool
    * mingw-w64
    * pbzip2 (optional, improves packaging times)
    * texinfo
* Mac OS X also requires Xcode.

Host/Target prebuilts
---------------------

### For Linux or Darwin:

```bash
$ python checkbuild.py
```

### For Windows, from Linux:

```bash
$ python checkbuild.py --system windows  # Or windows64.
```

`checkbuild.py` also accepts a variety of other options to speed up local
builds, namely `--arch` and `--module`.

Packaging
---------

By default, `checkbuild.py` will also package the NDK and run basic tests. To
skip the packaging step, use the `--no-package` flag. Note that running the
tests does require the packaging step.

If you need to re-run just the packaging step without going through a build,
packaging is handled by `build/tools/package.py`.

Testing
-------

### Single Configuration Testing

Running the NDK tests requires a complete NDK package (see previous steps).
From the NDK source directory (not the extracted package):

```bash
$ python tests/run-all.py --abi $ABI_TO_TEST path/to/ndk
```

To run the tests with GCC, use the option `--toolchain 4.9`.

The full test suite includes tests which run on a device or emulator, so you'll
need to have adb in your path and `ANDROID_SERIAL` set if more than one
device/emulator is connected. If you do not have a device capable of running the
tests, you can run just the `build` or `awk` test suites with the `--suite`
flag.

### Thorough Testing

There is also a script that will automatically detect connected
devices/emulators and choose a subset of them to fit our QA configuration. This
script will run all of the tests across all available ABIs on several devices,
and thus will take a long time (takes ~75 minutes on my machine, even with a few
configurations unavailable). As such, this isn't suitable for active
development, but should be run for QA and for any changes that might have a wide
impact (compiler updates, ndk-build changes, sysroot changes, etc).

To use this script, connect any devices and launch any emulators you need for
testing (make sure ADB has authorization to control them), then run:

```bash
$ python tests/validate.py path/to/ndk
```

By default, test logs will be placed in $CWD/test-logs. This can be controlled
with the `--log-dir` flag.

### Testing libc++

The libc++ tests are not currently integrated into the main NDK tests. To run
the libc++ tests:

```bash
$ NDK=/path/to/extracted/ndk sources/cxx-stl/llvm-libc++/llvm/ndk-test.sh $ABI
```

Note that these tests are far from failure free. In general, most of these tests
are locale related and fail because we don't support anything beyond the C
locale.
