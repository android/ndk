def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    # The ifunc only gets used on armeabi, by GCC. We support ifuncs for M+.
    if abi == 'armeabi' and toolchain == '4.9' and device_platform < 23:
        bug = 'https://github.com/android-ndk/ndk/issues/31'
        return '{} {} {}'.format(toolchain, abi, device_platform), bug
    return None, None
