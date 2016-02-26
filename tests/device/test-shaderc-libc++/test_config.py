def match_unsupported(abi, platform, toolchain, subtest=None):
    if abi == 'armeabi':
        return abi
    return None
