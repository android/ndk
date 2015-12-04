def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if abi not in ('armeabi-v7a', 'armeabi-v7a-hard'):
        return abi
    if device_platform < 18:
        return device_platform
    return None
