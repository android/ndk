Testing the NDK
===============


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
