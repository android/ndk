Generating NDK Sysroots
=======================

The `platforms/` subtree of an NDK package contains sysroots for each
architecture and API level. The contents of these sysroots are generated as part
of the NDK build.

The Sysroot Defintions
----------------------

The contents of the sysroots are defined by `development/ndk/platforms`.
Note that this is from the top level of the Android tree, not within the `ndk/`
subdirectory.

The layout of this tree is as follows:

 * `android-$VERSION`
     * `include`
     * `arch-$ARCH`
         * `include`
             * Architecture specific headers. Typically the `asm` and `machine`
               directories.
         * `lib`
             * Prebuilt static libraries.
             * Note that for the time being these are only libc, libm, libz, and
               libstdc++, and that they are rarely updated (we have versions for
               android-3, android-9, and android-21, but not any other
               releases).
         * `src`
             * Source for the CRT objects (crtbegin, crtend, etc.).
         * `symbols`
             * Contains interface definitions for the shared platform libraries.
             * Discussed in detail in [Shared Library Definitions].

These trees are a very reduced form of a sysroot. Not every header or library
will be in every platform version. Instead, [gen-platforms.sh] will build the
full sysroot from what is defined here. This is done to ease maintenance of the
headers and libraries. If, for example, `string.h` remained the same between
Gingerbread and KitKat, but was updated in Lollipop, there need only be a copy
of `string.h` in android-9 and android-21, not in every intermediate release.

Shared Library Defintions
-------------------------

When an NDK binary links against a platform library, rather than shipping a copy
of the library with the binary, it loads the library from the system itself (in
the case of an app, many of the libraries will already be loaded into the
address space by the zygote). As such, the shared libraries in the NDK do not
need to be valid aside from the symbols they expose. This simplifies things,
because all we need to generate the shared libraries for a given architecture
and platform version is a list of functions and variables.

The `android-$VERSION/arch-$ARCH/symbols/*.txt` files contain these library
definitions. Each file is named either `$LIBNAME.functions.txt` or
`$LIBNAME.variables.txt`. For each such file, [gen-platforms.sh] will create a
stub shared library for that library with the functions and variables defined in
those lists.

For some platform versions (currently only android-23), there is also a
`$LIBNAME.versions.txt`. This is a linker version script that will be used to
apply symbol versions when generating the stub shared library.

Examples of each of these three file types can be found in
[development/ndk/platforms/android-23/arch-arm64/symbols].

Adding a New Platform API
-------------------------

These instructions are valid both for adding new native frameworks APIs and
updating bionic.

 1. If the platform version you are adding your API to does not yet exist, there
    are no special steps; just make the new directory.
 2. Add your (possibly new, possibly updated) headers to the include directory.
 3. Add [Shared Library Definitions] files for each architecture to the
    `arch-$ARCH/symbols` directory.
 4. Add tests to [tests/device](../tests/device).
 5. Build and test an NDK (see [README.md](../README.md)).
 6. Upload your change(s) and add the current [NDK owners](../OWNERS) as
    reviewers.

[Shared Library Definitions]: #shared-library-defintions
[gen-platforms.sh]: ../build/tools/gen-platforms.sh
[development/ndk/platforms/android-23/arch-arm64/symbols]: https://android.googlesource.com/platform/development/+/master/ndk/platforms/android-23/arch-arm64/symbols
