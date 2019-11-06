## Betas, release candidates, canary releases, and stable releases

Stable releases along with any current betas and release candidates are available from this page.

Canary releases are available on [ci.android.com](https://ci.android.com/builds/branches/aosp-master-ndk/grid?) for all supported operating systems.

Note: canaries, betas, and release candidates should not be used for production, but are a great way for you to find and report bugs early, before they make it to a stable release.

## Announcements

Subscribe to the [android-ndk-announce@googlegroups.com](https://groups.google.com/g/android-ndk-announce) mailing list if you'd like to be notified whenever we publish a new NDK release (beta or stable).

## macOS 10.15 "Catalina"

We're currently aiming to sign and notarize NDK r21, but no existing NDK is signed or notarized. This shouldn't matter for NDK downloads performed by Android Studio's SDK Manager, but if you manually download an NDK from this page using a web browser, on macOS 10.15 you'll see warnings when you try to run the extracted tools. You can override this in the "Security & Privacy" section of Settings. See Apple's [Safely open apps on your Mac](https://support.apple.com/en-us/HT202491) documentation, specifically the section "How to open an app that hasn’t been notarized or is from an unidentified developer" on that page.

## Release Schedule

The NDK releases on a roughly quarterly basis. One release a year is designated
as a Long Term Support (LTS) release that will receive backports until the next
LTS is released. Each version is first released as a beta and then a release
candidate before being shipped to stable. Multiple betas and release candidates
are a possibility if the first beta does not meet our promotion criteria, but
there will always be at least one of each. For details, see [NDK Release
Process](/android/ndk/wiki/NDK-Release-Process).

### NDK r21 (LTS)

Release | Expected Release Date
------- | ---------------------
Beta 1  | Released!
RC 1    | TBD
Release | TBD

## Supported Downloads

### Current Beta Release

[[r21 Changelog|Changelog-r21]]

<table>
  <tr>
    <th>Platform</th>
    <th>Package</th>
    <th>Size (bytes)</th>
    <th>SHA1 Checksum</th>
  </tr>
  <tr>
    <td>Linux</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r21-linux-x86_64.zip">android-ndk-r21-linux-x86_64.zip</a></td>
    <td>1090223430</td>
    <td>9c20b1095124f15a8ed3f7349f1ccb69ec66e891</td>
  </tr>
  <tr>
    <td>Mac</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r21-darwin-x86_64.zip">android-ndk-r21-darwin-x86_64.zip</a></td>
    <td>1002920052</td>
    <td>14b45877e6e09d1c88c75709b3dcaf8055f16efd</td>
  </tr>
  <tr>
    <td>Windows</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r21-windows-x86_64.zip">android-ndk-r21-windows-x86_64.zip</a></td>
    <td>1057163708</td>
    <td>b06620b1fbc5c9be93ab6d34e2051cf72c08b56d</td>
  </tr>
</table>

### Current Stable Release

[[r20 Changelog|Changelog-r20]]

<table>
  <tr>
    <th>Platform</th>
    <th>Package</th>
    <th>Size (bytes)</th>
    <th>SHA1 Checksum</th>
  </tr>
  <tr>
    <td>Linux</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r20b-linux-x86_64.zip">android-ndk-r20b-linux-x86_64.zip</a></td>
    <td>859780564</td>
    <td>d903fdf077039ad9331fb6c3bee78aa46d45527b</td>
  </tr>
  <tr>
    <td>Mac OS X</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r20b-darwin-x86_64.zip">android-ndk-r20b-darwin-x86_64.zip</a></td>
    <td>843201217</td>
    <td>b51290ab69cb89de1f0ba108702277bc333b38be</td>
  </tr>
  <tr>
    <td>Windows 32-bit</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r20b-windows-x86.zip">android-ndk-r20b-windows-x86.zip</a></td>
    <td>814464692</td>
    <td>71a1ba20475da1d83b0f1a1826813008f628d59b</td>
  </tr>
  <tr>
    <td>Windows 64-bit</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r20b-windows-x86_64.zip">android-ndk-r20b-windows-x86_64.zip</a></td>
    <td>832473103</td>
    <td>ead0846608040b8344ad2bc9bc721b88cf13fb8d</td>
  </tr>
</table>
