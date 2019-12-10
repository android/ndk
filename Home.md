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
Beta 2  | Released!
RC 1    | Released!
Release | January 2020

## Supported Downloads

### Current Release Candidate

[[r21 Changelog|Changelog-r21]]

Note: r21 RC1 is the same build as beta 2.

<table>
  <tr>
    <th>Platform</th>
    <th>Package</th>
    <th>Size (bytes)</th>
    <th>SHA1 Checksum</th>
  </tr>
  <tr>
    <td>Linux</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r21-beta2-linux-x86_64.zip">android-ndk-r21-beta2-linux-x86_64.zip</a></td>
    <td>1043467643</td>
    <td>335f30302bee700a9a5fdfe3ae533a4963499c44</td>
  </tr>
  <tr>
    <td>Windows</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r21-beta2-windows-x86_64.zip">android-ndk-r21-beta2-windows-x86_64.zip</a></td>
    <td>1057424857</td>
    <td>e5902e4f6c5b6f0354a2572a85f42f19c1a7b9d8</td>
  </tr>
  <tr>
    <td>macOS</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r21-beta2-darwin-x86_64.zip">android-ndk-r21-beta2-darwin-x86_64.zip</a></td>
    <td>958791657</td>
    <td>34a46c3867c9d87a80895c0b8a098256052536d2</td>
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
