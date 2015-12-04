def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if abi == 'armeabi-v7a-hard' and device_platform <= 16:
        return '{} {}'.format(abi, device_platform), 'http://b/26014163'
    return None, None
