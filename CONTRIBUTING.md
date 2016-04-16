Contributing to the Android NDK
===============================

This guide will help you contribute to the Android NDK, whether by reporting
issue or submitting a patch.

Reporting Issues
----------------

Our bug tracker is hosted on GitHub: https://github.com/android-ndk/ndk/issues.

**Note:** Bugs against Android Studio or Gradle should be filed at
http://b.android.com, not here.

Before filing a bug, check that you are using the latest version of the NDK. The
latest stable version and the preview release of the next stable version are
listed on our wiki: https://github.com/android-ndk/ndk/wiki#downloads.

Submitting Patches
------------------

While we use GitHub for our bug tracking and documentation, we do not manage our
code in GitHub. Like the rest of Android, we are hosted in AOSP. This means that
patches are uploaded to and submitted via Gerrit.

Follow the [contribution instructions for
AOSP](http://source.android.com/source/submit-patches.html). When you run `repo
init`, follow the instructions in the NDK's [README.md]. We have a different set
of dependencies than AOSP, also noted in our [README.md].

Instructions for building the NDK and running our tests are documented in
[README.md] and [Testing.md](docs/Testing.md). Note that the NDK cannot be built
on Windows; only Linux and Darwin hosts are supported.

Check the [OWNERS file](OWNERS) for reviewers.

[README.md]: README.md
