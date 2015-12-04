def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if device_platform <= 10:
        return device_platform, 'http://b/26015756'
    return None, None
