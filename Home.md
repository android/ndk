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

### Current LTS Release

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
    <td>1043332542</td>
    <td>afc9c0b9faad222898ac8168c78ad4ccac8a1b5c</td>
  </tr>
  <tr>
    <td>Windows</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r21-windows-x86_64.zip">android-ndk-r21-windows-x86_64.zip</a></td>
    <td>1057294207</td>
    <td>c61631eacbd40c30273b716a4e589c6877b85419</td>
  </tr>
  <tr>
    <td>macOS</td>
    <td><a href="https://dl.google.com/android/repository/android-ndk-r21-darwin-x86_64.zip">android-ndk-r21-darwin-x86_64.zip</a></td>
    <td>958656601</td>
    <td>0d50636cc0e34ed3ba540d6d5818ea0cf10f16aa</td>
  </tr>
</table>
