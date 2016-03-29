Working with the NDK Toolchains
===============================

The toolchains shipped in the NDK are not built as a part of the NDK build
process. Instead they are built separately and checked into git as prebuilts
that are repackaged when shipped in the NDK. This applies to both Clang and GCC.

Both toolchains are built separately. An artifact of the build is a tarball of
the compiler for distribution. That artifact is unpacked into a location in the
Android tree and checked in. The NDK build step for each toolchain simply packs
up the contents of the directory.

Note: Any changes to either toolchain need to be tested in the platform *and*
the NDK. The platform and the NDK both get their toolchains from the same build.

This process is far too manual. `checkbuild.py` should be updated (or additional
scripts added) to ease this process.

Clang
-----

Clang's build process is described in the platform's [ReadmeAndroid.md]. Note
that Clang cannot be built from the NDK tree. The output tarball is extracted to
`prebuilts/clang/host/$HOST/clang-$BUILD_NUMBER`.  `checkbuild.py --module
clang` repackages this as-is as `$OUT_DIR/dist/llvm-$HOST_TAG.zip`.

[ReadmeAndroid.md]: https://android.googlesource.com/platform/external/clang/+/dev/ReadmeAndroid.md

### Testing Local Changes

To test a Clang you just built:

```bash
$ export CLANG_PREBUILTS=`realpath ../prebuilts/clang/host/linux-x86`
$ rm -r $CLANG_PREBUILTS/clang-dev
$ tar xf path/to/clang-dev-linux-x86_64.tar.bz2 -C $CLANG_PREBUILTS
# Edit ndk/build/tools/build-llvm.py and change the version to 'clang-dev'.
$ ../prebuilts/ndk/symlink-clang.py dev
$ ./checkbuild.py
# Run tests.
```

For details about running tests, see [Testing.md].

[Testing.md]: Testing.md

This installs the new Clang into the prebuilts directory so it can be packaged
by the NDK build. The `symlink-clang.py` line updates the symlinks in prebuilts
NDK to point at the new Clang. The Clang in `prebuilts/ndk` is used for building
libc++ in the NDK. The difference between it and `prebuilts/clang` is the
directory layout, which differs so that `ndk-build` can use it.

If you need to make changes to Clang after running the above steps, future
updates can be done more quickly with:

```bash
$ rm -r $CLANG_PREBUILTS/clang-dev
$ tar xf path/to/clang-dev-linux-x86_64.bz2 -C $CLANG_PREBUILTS
$ ./checkbuild.py --module clang --force-package
# Run tests.
```

We don't need to rebuild the whole NDK since we've already built most of it.
Note that any NDK modules that were built with Clang won't be rebuilt (e.g.
libc++), so this is only going to be useful if you're debugging a test.

### Updating to a New Clang

These steps need to be run after installing the new prebuilt from the build
server to `prebuilts/clang` (see the Clang docs).

```bash
# Edit ndk/build/tools/build-llvm.py and update the build number.
$ ../prebuilts/ndk/symlink/clang.py # Latest version autodetected.
$ ./checkbuid.py
# Run tests.
```

GCC (and binutils)
------------------

GCC's build process is described in [GCC's README.md][GCCReadme]. Unlike Clang,
GCC can be built from the NDK tree. The output tarball is extracted to
`prebuilts/ndk/current/toolchains/$TOOLCHAIN/prebuilt/$HOST_TAG`. `checkbuild.py
--module gcc` repackages this as-is as `$OUT_DIR/dist/gcc-$ARCH-$HOST_TAG.zip`.

Note that GCC is a bit more complicated to update than Clang. `prebuilts/ndk`
contains only a prebuilt GCC, not prebuilt target gnustl (GNU's libstdc++,
referred to as gnustl in Android). As such, changes submitted to `toolchain/gcc`
will affect the gnustl build (`checkbuild.py --module gnustl`) will take effect
immediately.

[GCCReadme]: https://android.googlesource.com/toolchain/gcc/+/master/README.md

### Testing Local Changes

To test a GCC you just built:

```bash
$ export INSTALL_DIR=`realpath ../prebuilts/ndk/current`
$ export TOOLCHAIN_DIR=$INSTALL_DIR/toolchains/$HOST_TAG
$ rm -r $TOOLCHAIN_PATH/$TOOLCHAIN
$ unzip ../out/dist/gcc-$ARCH-$HOST.zip -d $TOOLCHAIN_DIR
$ ./checkbuild.py
# Run tests.
```

We're building an NDK and then replacing it's GCC with our own.

For details about running tests, see [Testing.md].

Additional changes will not require a full `checkbuild.py`. Instead:

```bash
$ ./checkbuild.py --module gcc
$ ./checkbuild.py --module gcclibs
$ ./checkbuild.py --module gnustl --force-package
# Run tests.
```

### Updating to a New GCC

```bash
$ ../prebuilts/ndk/update-gcc.py $BUILD_NUMBER
$ ./checkbuild.py
# Run tests.
```
