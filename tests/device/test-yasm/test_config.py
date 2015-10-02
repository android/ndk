def match_unsupported(abi, platform, toolchain, subtest=None):
    if abi not in ('x86', 'x86_64'):
        return abi
    return None
