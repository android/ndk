def match_unsupported(abi, platform, toolchain, subtest=None):
    if abi not in ('armeabi-v7a', 'armeabi-v7a-hard', 'x86'):
        return abi
    return None
