def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if abi not in ('armeabi-v7a-hard', 'arm64-v8a', 'x86', 'x86_64'):
        return abi
    return None

