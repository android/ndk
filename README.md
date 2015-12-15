Android Native Development Kit (NDK)
====================================

The NDK allows Android application developers to include
native code in their Android application packages, compiled as JNI shared
libraries.

Discussions related to the Android NDK happen on the
[android-ndk](http://groups.google.com/group/android-ndk) Google Group.

Building the NDK
================

**Note:** This document is for developers _of_ the NDK, not developers
that use the NDK.

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
* `binutils/` contains the standalone binutils installation for use with Clang.
* `host-tools/` contains build dependencies and additional tools.
    * make, awk, python, yasm, and for Windows: cmp.exe and echo.exe
    * `ndk-depends`, `ndk-stack` and `ndk-gdb` can also be found here.

### Target Headers and Binaries

* `platforms/android-N/arch-$ARCH_NAME/` contains headers and libraries for each
  API level.
    * The build system sets `--sysroot` to one of these directories based on
      user-specified `APP_ABI` and `APP_PLATFORM`.
* `sources/cxx-stl/$STL` contains the headers and libraries for the various C++
  STLs.
* `gdbserver/` contains gdbserver.

### Others

* `build/` contains the ndk-build system and scripts to rebuild NDK.
* `docs/`
* `sources/` contains modules useful in samples and apps via
  `$(call import-module, $MODULE)`
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
    * texinfo
    * gcc-mingw32
    * wine
    * bison
    * flex
    * dmake
    * libtool
    * pbzip2
* Mac OS X also requires Xcode.

Host/Target prebuilts
---------------------

### For Linux or Darwin:

```bash
$ python checkbuild.py --no-package
```

### For Windows, from Linux:

```bash
$ python checkbuild.py --system windows
```

`checkbuild.py` also accepts a variety of other options to speed up local
builds, namely `--arch` and `--module`.

Packaging
---------

The simplest way to package an NDK on Linux is to just omit the `--no-package`
flag when running `checkbuild.py`. This will take a little longer though, so it
may not be desired for day to day development.

If you need to re-run just the packaging step without going through a build,
packaging is handled by `build/tools/package.py`.

Testing
-------

Running the NDK tests requires a complete NDK package (see previous steps).
From the NDK source directory (not the extracted package):

```bash
$ NDK=/path/to/extracted/ndk python tests/run-all.py --abi $ABI_TO_TEST
```

To run the tests with Clang, use the option `--toolchain clang`.

The full test suite includes tests which run on a device or emulator, so you'll
need to have adb in your path and `ANDROID_SERIAL` set if more than one
device/emulator is connected. If you do not have a device capable of running the
tests, you can run just the `build` or `awk` test suites with the `--suite`
flag.

The libc++ tests are not currently integrated into the main NDK tests. To run
the libc++ tests:

```bash
$ NDK=/path/to/extracted/ndk sources/cxx-stl/llvm-libc++/llvm/ndk-test.sh $ABI
```

Note that these tests are far from failure free (especially on 32-bit ARM). In
general, most of these tests are locale related and fail because we don't
support anything beyond the C locale. The ARM32 specific failures are because
the libgcc unwinder does not get along with the LLVM unwinder. The test config
file (`$NDK/sources/cxx-stl/llvm-libc++/libcxx/test/libcxx/ndk/test/config.py`)
can be modified to use `-lc++_static` *before* `-lgcc` and the tests will then
work on ARM (but will take considerably longer to run).

Yes, this does mean that exception handling will often fail when using
`c++_shared` on ARM32. We should fix this ASAP, but this actually is not a
regression from r10e.
