# ndk-gdb testing

#### Setup steps:
1. Sync the Android.mk samples with `git clone -b android-mk https://github.com/googlesamples/android-ndk.git`
2. Pick a sample app (e.g. Teapot), cd into its directory, and compile and install it to a device with `android update project -p . --target <target> && ndk-build && ant clean debug install`, where `<target>` is an SDK target such as `android-23`.
3. Run ndk-gdb.py. This can be done in three ways:
    1. Launch the application, and then run `ndk-gdb` from the app directory to attach the debugger.
    2. Run `ndk-gdb --launch` to have ndk-gdb start the application's main activity.
    3. Run `ndk-gdb --launch-activity <activity name>` to have ndk-gdb start a specific activity.
4. Test things!

#### Relevant things to test:
1. Verify that backtraces are sensible.
2. Verify that breakpoints set before and after library load both work. (Using --launch vs. attaching after the application is running helps here.)
3. Verify that C++ stdlib pretty printers work.
4. Verify that ndk-gdb works with all valid combinations of app ABI support (defined in APP_ABI in Application.mk) and device ABI support. For example, test with the following combinations:
    1. 32-bit device:
        1. `APP_ABI := armeabi`
        2. `APP_ABI := armeabi arm64-v8a`
    2. 64-bit device:
        1. `APP_ABI := armeabi`
        2. `APP_ABI := armeabi arm64-v8a`
        3. `APP_ABI := arm64-v8a`
