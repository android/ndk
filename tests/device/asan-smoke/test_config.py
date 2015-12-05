def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if not toolchain.startswith('clang'):
        return toolchain
    if not abi.startswith('armeabi'):
        return abi
    if device_platform < 19:
        return device_platform
    return None
