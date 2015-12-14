def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if abi == 'x86':
        return abi, 'http://b/25981507'
    return None, None


def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if not toolchain.startswith('clang'):
        return toolchain
    if not abi.startswith('armeabi') and not abi == 'x86':
        return abi
    if device_platform < 19:
        return device_platform
    return None
