**Note**: Bugs against Android Studio or Gradle should be filed at
http://b.android.com, not here.

Before filing a bug, check that you are using the latest version of the NDK. The
latest stable version and the preview release of the next stable version are
listed on our wiki: https://github.com/android-ndk/ndk/wiki#downloads.

Make sure your issue isn't on the list of common problems before filing a bug:
https://android.googlesource.com/platform/ndk/+/master/docs/user/common_problems.md

Delete everything above this line before submitting.

--------------------------------------------------------------------------------

#### Description

Bug description goes here. Include a test case. It's unlikely that anything can
be done without a test case.

Bug reports with better test cases will be resolved sooner than those with bad
test cases. A good test case:

 * Is as small as possible while still exposing the bug.
 * Does not require Android Studio or Gradle (if your bug cannot be reproduced
   without either of those, it's a bug in Studio or Gradle, and should be
   reported to Android Studio).
 * Is complete. Snippets that would not compile even in the absence of the bug
   you are reporting slow down the process.

Here's an example of a good test case:
https://github.com/DanAlbert/dynamic-cast-repro

#### Environment Details

Not all of these will be relevant to every bug, but please provide as much
information as you can.

* NDK Version: Value from Pkg.Revision in ndk-bundle/source.properties. If not
  the latest version, check there before filing.
* Build sytem: ndk-build? cmake? standalone toolchain? custom? (if gradle, file
  a bug against studio as specified above)
* Host OS: Windows, Mac, or Linux? Which Linux?
* Compiler: If GCC, check Clang before filing. GCC is no longer supported.
* ABI:
* STL:
* NDK API level:
* Device API level:
