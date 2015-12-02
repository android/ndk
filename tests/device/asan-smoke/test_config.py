def match_unsupported(abi, platform, toolchain, subtest=None):
    if not toolchain.startswith('clang'):
        return toolchain
    if not abi.startswith('armeabi'):
        return abi
    return None
