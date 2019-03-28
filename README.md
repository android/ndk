Android Native Development Kit (NDK)
====================================

The NDK allows Android application developers to include
native code in their Android application packages, compiled as JNI shared
libraries.

 * User documentation is available on the [Android Developer website](https://developer.android.com/ndk/):
   * The [NDK Guides](https://developer.android.com/ndk/guides/) help you get started with the NDK.
     * Information about [building](https://developer.android.com/ndk/guides/build)
       and [debugging and profiling](https://developer.android.com/ndk/guides/debug).
     * Tutorial information for
       [High-Performance Audio](https://developer.android.com/ndk/guides/audio),
       [Vulkan](https://developer.android.com/ndk/guides/graphics), and
       [Neural Networks](https://developer.android.com/ndk/guides/neuralnetworks).
   * The [NDK API Reference](https://developer.android.com/ndk/reference) covers all Android-specific APIs.
 * The documentation for Android's C library ("bionic") may be useful:
   * [Android bionic status](https://android.googlesource.com/platform/bionic/+/master/docs/status.md) talks
     about what APIs are available in what releases, and behaviors that have changed between API levels.
   * [Android changes for NDK developers](https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md)
     details important dynamic linker changes in various Android releases. Invaluable if
     you're having trouble loading your .so files.
 * Discussions related to the Android NDK happen on the [android-ndk Google
   Group](http://groups.google.com/group/android-ndk).
 * File bugs against the NDK at https://github.com/android-ndk/ndk/issues.
 * See our [calendar for upcoming releases](https://github.com/android-ndk/ndk/wiki).


If you want to work _on_ the NDK itself rather than just use it, see the NDK team's
[internal documentation](https://android.googlesource.com/platform/ndk/+/master/README.md).
