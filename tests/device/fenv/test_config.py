def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if abi == 'armeabi':
        return abi
    return None
