Android Native Development Kit (NDK)
====================================

The NDK allows Android application developers to include
native code in their Android application packages, compiled as JNI shared
libraries.

See [the Getting Started Guide](docs/Getting_Started/html/index.html) for an
introduction.

See [the changelist](docs/Change_Log.html) for a list of changes since the
previous release.

Finally, discussions related to the Android NDK happen on the
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

* `toolchains/` contains GCC, Clang, and Renderscript toolchains.
    * `$TOOLCHAIN/config.mk` contains ARCH and ABIS this toolchain can handle.
    * `$TOOLCHAIN/setup.mk` contains toolchain-specific default CFLAGS/LDFLAGS
      when this toolchain is used.
* `prebuilt/$HOST_ARCH/` contains various tools to make the build system hermetic.
    * make, awk, sed, perl, python, yasm, and for Windows: cmp.exe and echo.exe
* `prebuilt/common` contains the `scan-build` and `scan-view` scripts used by
  static analyzer via `NDK_ANALYZE=1`.
* `ndk-depends` and `ndk-stack` should probably go in `prebuilt/` to avoid
  collisions between host variants.


### Target Headers and Binaries

* `platforms/android-N/arch-$ARCH_NAME/` contains headers and libraries for each
  API level.
    * The build system sets `--sysroot` to one of these directories based on
      user-specified `APP_ABI` and `APP_PLATFORM`.
* `sources/cxx-stl/$STL/$ABI/` contains the headers and libraries for the various
  C++ STLs.
* `prebuilt/android-$ARCH/gdbserver/` contains gdbserver.

### Others

* `build/` contains the ndk-build system and scripts to rebuild NDK.
* `docs/`
* `samples/`
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

    * The only difference between the NDK branch and master is that the NDK
      repository already has the toolchain repository checked out and patched.

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

Packaging
---------

The simplest way to package an NDK on Linux is to just omit the `--no-package`
flag when running `checkbuild.py`. This will take a little longer though, so it
may not be desired for day to day development.

To package the NDK for Windows or Darwin (or if more control over the packaging
process is needed), invoke `build/tools/package-release.sh` directly. This
process will be improved in a future commit.

Best Practices for Incremental Builds
-------------------------------------

* Do not run `build/tools/dev-cleanup.sh` which erases all prebuilts.
* Remove the individual prebuilt directory if new build deletes files. This is
  rare.
* Package NDK from prebuilt tarballs in `$PREBUILT_PATH`. It runs
  `build/tools/build-docs.sh` which produces no tarball in `$PREBUILT_PATH`.
* Unpack and compare against the previous package.
