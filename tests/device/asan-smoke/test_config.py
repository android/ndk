def match_broken(abi, platform, toolchain, subtest=None):
    return toolchain, 'http://b/25792040'


def match_unsupported(abi, platform, toolchain, subtest=None):
    if not toolchain.startswith('clang'):
        return toolchain
    return None
