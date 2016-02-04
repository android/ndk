def match_unsupported(abi, platform, toolchain, subtest=None):
    if abi != 'mips':
        return abi
    return None


def match_broken(abi, platform, toolchain, subtest=None):
    if toolchain == 'clang':
        return toolchain, 'http://b/26984979'
    return None, None
