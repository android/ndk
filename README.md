Android Native Development Kit (NDK)
====================================

The latest version of this document is available at
https://android.googlesource.com/platform/ndk/+/master/README.md.

**Note:** This document is for developers _of_ the NDK, not developers
that use the NDK.

The NDK allows Android application developers to include
native code in their Android application packages, compiled as JNI shared
libraries.

Other Resources
---------------

This doc gives a high level overview of the NDK's build, packaging, and test
process. For other use cases, or more in depth documentation, refer to the
following sources:

 * User documentation is available on the [Android Developer website].
 * Adding a new NDK API or platform version? Check [Generating Sysroots].
 * Working on Clang or GCC? See [Toolchains.md].
 * Discussions related to the Android NDK happen on the [android-ndk Google
   Group].
 * File bugs against the NDK at https://github.com/android-ndk/ndk/issues.

[Android Developer website]: https://developer.android.com/ndk/index.html
[android-ndk Google Group]: http://groups.google.com/group/android-ndk
[Generating Sysroots]: docs/GeneratingSysroots.md
[Toolchains.md]: docs/Toolchains.md

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
    * python3 (used for Vulkan validation layer generation)
    * python-lxml (used for Vulkan validation layer generation)
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

Testing is discussed in [Testing.md](docs/Testing.md).
