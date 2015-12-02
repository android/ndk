def match_unsupported(abi, platform, toolchain, subtest=None):
    if not toolchain.startswith('clang'):
        return toolchain
    return None
