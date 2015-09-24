def match_unsupported(abi, platform, toolchain, subtest=None):
    if abi != 'mips':
        return abi
    return None
