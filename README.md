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
binaries is built via MinGW cross compiler. Systems without a working MinGW
compiler can use `build/tools/build-mingw64-toolchain.sh` to generate their own
and be added to the `PATH` for build scripts to discover.

Mac OS X host binaries should be built on the minimal Mac OS that the NDK
supports (currently 10.8).

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
* `prebuilt/$HOST_ARCH/` contains various tools to make build system hermetic.
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
* `sources/android/compiler-rt/libs/` contains compiler-rt libraries for use
  with Clang.
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

* [AOSP Platform Repository](http://source.android.com/source/downloading.html)
    * Only `ndk/` and `development/` are actually needed.
* Toolchain Repository
    * Contains binutils, GCC, LLVM/Clang, etc.
    * To initialize:

        ```bash
        $ build/tools/download-toolchain-sources.sh desired/toolchain/src/dir
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

### On Linux:

```bash
$ export AOSP=/absolute/path/to/aosp
$ export NDK=$AOSP/ndk
$ export PREBUILT_PATH=/absolute/path/to/packaged/output
$ cd $NDK
$ build/tools/dev-cleanup.sh
$ build/tools/download-toolchain-sources.sh /tmp/ndk-$USER/src
$ build/tools/rebuild-all-prebuilt.sh /tmp/ndk-$USER/src \
    --package-dir=$PREBUILT_PATH --also-64 --systems=linux-x86,windows
```

### On MacOS X:

```bash
$ export AOSP=/absolute/path/to/aosp
$ export NDK=$AOSP/ndk
$ export PREBUILT_PATH=/absolute/path/to/packaged/output
$ cd $NDK
$ build/tools/dev-cleanup.sh
$ build/tools/build-host-prebuilts.sh --ndk-dir=$NDK \
    --package-dir=$PREBUILT_PATH --systems=darwin-x86 /tmp/ndk-$USER/src \
    --try-64
$ build/tools/build-host-prebuilts.sh --ndk-dir=$NDK \
    --package-dir=$PREBUILT_PATH --systems=darwin-x86 /tmp/ndk-$USER/src
```

Packaging
---------

Once all prebuilt tarballs are at `$PREBUILT_PATH`, run the following to package
the NDK:

```bash
$ build/tools/package-release.sh --prebuilt-dir=/s/prebuilt --separate-64 \
    --release=r9x
```

Best Practices for Incremental Builds
-------------------------------------

* Do not run `build/tools/dev-cleanup.sh` which erases all prebuilts.
* Remove the individual prebuilt directory if new build deletes files. This is
  rare.
* Avoid running `build/tools/download-toolchain-sources.sh` during development
  because the size of toolchains repo is ~8GB. Changes made to individual
  projects can be cherry-picked to `/tmp/ndk-$USER/src`.
* Package NDK from prebuilt tarballs in `$PREBUILT_PATH`. It runs
  `build/tools/build-docs.sh` which produces no tarball in `$PREBUILT_PATH`.
* Unpack and compare against the previous package.
