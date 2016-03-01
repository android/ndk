def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if device_platform < 18:
        return device_platform
    return None
