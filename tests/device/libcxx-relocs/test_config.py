def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if abi == 'armeabi' and toolchain == '4.9':
        bug = 'https://github.com/android-ndk/ndk/issues/31'
        return toolchain + ' ' + abi, bug
    return None, None
