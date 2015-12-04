def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if device_platform <= 10:
        return device_platform, 'http://b/26016050'
    return None


def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if abi not in ('armeabi-v7a', 'armeabi-v7a-hard'):
        return abi
    return None
