# Android Native Development Kit (NDK)

The NDK allows Android application developers to include
native code in their Android application packages, compiled as JNI shared
libraries.

For what's coming in NDK r21 in 2019Q3, see the [r21 hotlist](https://github.com/android-ndk/ndk/milestone/20).

For further into the future, see the [NDK Roadmap](https://android.googlesource.com/platform/ndk/+/master/docs/Roadmap.md).

## RFC

This section lists any in-progress features with open discussion bugs. We're
still working on these and want to hear from you, so please read the thread and
join the discussion if you have anything to add!

 * NDK package management: https://github.com/android-ndk/ndk/issues/916

## NDK documentation

Tutorial and API reference documentation is available on the [Android Developer website](https://developer.android.com/ndk/):
   * The [NDK Guides](https://developer.android.com/ndk/guides/) help you get started with the NDK.
     * Information about [building](https://developer.android.com/ndk/guides/build)
       or [debugging and profiling](https://developer.android.com/ndk/guides/debug).
     * Tutorial information for
       [High-Performance Audio](https://developer.android.com/ndk/guides/audio),
       [Vulkan](https://developer.android.com/ndk/guides/graphics), and
       [Neural Networks](https://developer.android.com/ndk/guides/neuralnetworks).
   * The [NDK API Reference](https://developer.android.com/ndk/reference) covers all Android-specific APIs.

## C library ("bionic") and dynamic linker documentation

The documentation for Android's C library ("bionic") may be useful:
   * [Android bionic status](https://android.googlesource.com/platform/bionic/+/master/docs/status.md) talks
     about what APIs are available in what releases, and behaviors that have changed between API levels.
   * [Android changes for NDK developers](https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md)
     details important dynamic linker changes in various Android releases. Invaluable if
     you're having trouble loading your .so files.
   * [32-bit ABI bugs](https://android.googlesource.com/platform/bionic/+/master/docs/32-bit-abi.md)
     documents issues for 32-bit code.

## Understanding crashes/tombstones

The documentation for Android OS developers has:
   * An overview of [crash dumps/tombstones](https://source.android.com/devices/tech/debug).
   * A detailed reference to understanding [common crashes](https://source.android.com/devices/tech/debug/native-crash).

## Other resources

 * Discussions related to the Android NDK happen on the [android-ndk Google
   Group](http://groups.google.com/group/android-ndk).
 * File bugs against the NDK at https://github.com/android-ndk/ndk/issues.
 * See our [calendar for upcoming releases](https://github.com/android-ndk/ndk/wiki).
 * The NDK and Android Studio support ndk-build and CMake out of the box, but we also have a
   [Build System Maintainers Guide](https://android.googlesource.com/platform/ndk/+/master/docs/BuildSystemMaintainers.md).
 * If you want to work _on_ the NDK itself rather than just use it, see the NDK team's
[internal documentation](https://android.googlesource.com/platform/ndk/+/master/README.md).
