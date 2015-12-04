def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if abi == 'armeabi':
        # I have no interest in supporting rebuilding libc++ for ARM5.
        return abi
    return None
