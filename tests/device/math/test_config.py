def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if abi == 'x86_64' and toolchain == 'clang':
        bug = 'http://b.android.com/177432'
        return '{} {} (ld128)'.format(toolchain, abi), bug
    return None, None


def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if device_platform < 18:
        return device_platform
    return None
