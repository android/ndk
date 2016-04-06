Testing the NDK
===============

The latest version of this document is available at
https://android.googlesource.com/platform/ndk/+/master/docs/Testing.md.

Testing Tools
-------------

There are currently three tools used in testing:

 1. `run-all.py`, for testing a specific configuration.
 2. `validate.py`, for testing many configurations.
 3. `ndk-test.sh`, for testing libc++.

A test configuration is a tuple of (ABI, target platform, toolchain, device).

At some point the three of these will most likely merge into one script.

### Testing a Single Configuration: [run-all.py]

For targeted testing during development, `run-all.py` can be used to verify a
single test configuration, as well as run specific tests.

Running the NDK tests requires a complete NDK package (see [README.md] for
building instructions).

[README.md]: ../README.md

From the NDK source directory (not the extracted package):

```bash
$ python tests/run-all.py --abi $ABI_TO_TEST path/to/ndk
```

The default toolchain for testing is Clang. To run the tests with GCC, use the
option `--toolchain 4.9`.

The full test suite includes tests which run on a device or emulator, so you'll
need to have `adb` in your path and `ANDROID_SERIAL` set if more than one
device/emulator is connected. If you do not have a device capable of running the
tests, you can run just the `build` or `awk` test suites with the `--suite`
flag.

### Testing Multiple Configurations: [validate.py]

When testing multiple configurations, `validate.py` will automatically detect
connected devices/emulators and choose a subset of them to fit our QA
configuration. This script will run all of the tests across all available ABIs
on several devices, and thus will take a long time (takes ~75 minutes on my
machine, even with a few configurations unavailable). As such, this isn't
suitable for active development, but should be run for QA and for any changes
that might have a wide impact (compiler updates, `ndk-build` changes, sysroot
changes, etc).

To use this script, connect any devices and launch any emulators you need for
testing (make sure ADB has authorization to control them), then run:

```bash
$ python tests/validate.py path/to/ndk
```

By default, test logs will be placed in $PWD/test-logs. This can be controlled
with the `--log-dir` flag.

### Testing libc++: [ndk-test.sh]

The libc++ tests are not currently integrated into the main NDK tests. To run
the libc++ tests:

```bash
$ NDK=/path/to/extracted/ndk sources/cxx-stl/llvm-libc++/libcxx/ndk-test.sh $ABI
```

Note that these tests are far from failure free. In general, most of these tests
are locale related and fail because we don't support anything beyond the C
locale.

Setting Up a Test Environment
-----------------------------

To run the NDK tests, you will need:

 * An NDK. The NDK doesn't necessarily need to contain support for every
   architecture.
 * `adb` in your path.
     * This is only needed if you're running device tests.
     * Always use the latest available version of `adb`. Note that the version
       of `adb` in the SDK manager might be surprisingly old. It's best to use a
       version built fresh from AOSP.
 * A device or emulator attached.
     * Again, only needed for device tests.

### Devices and Emulators

For testing a release, make sure you're testing against the released builds of
Android.

For Nexus devices, factory images are available here:
https://developers.google.com/android/nexus/images. Googlers, you may want to
use the flash station to get a userdebug image since that is needed for ASAN
testing. You should still make sure you also test on user builds because that is
what all of our users have.

For emulators, use emulator images from the SDK rather than from a platform
build. Again, these are what our users will be using.

After installing the emulator images from the SDK manager, they can be
configured and launched for testing with (assuming the SDK tools directory is in
your path):

```bash
$ android create avd --name $NAME --target android-$LEVEL --abi $ABI
$ emulator -avd $NAME -no-window -no-audio -no-skin
```

This will create a new virtual device and launch it in a headless state.

QA Configuration
----------------

The current configuration we use to test NDK releases is as written in
[validate.py]:

```python
{
    10: {
        'armeabi': None,
        'armeabi-v7a': None,
        'armeabi-v7a-hard': None,
    },
    16: {
        'armeabi': None,
        'armeabi-v7a': None,
        'armeabi-v7a-hard': None,
    },
    23: {
        'armeabi': None,
        'armeabi-v7a': None,
        'armeabi-v7a-hard': None,
        'arm64-v8a': None,
        'x86': None,
        'x86_64': None,
    },
}
```

Each API level/ABI pair will be checked with both Clang and GCC.

Note that there are no ARM64 emulators whatsoever in the SDK manager. Testing
ARM64 will require a physical device.

[run-all.py]: ../tests/run-all.py
[validate.py]: ../tests/validate.py
[ndk-test.sh]: ../sources/cxx-stl/llvm-libc++/libcxx/ndk-test.sh
