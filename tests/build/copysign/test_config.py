def match_broken(abi, platform, toolchain, subtest=None):
    if abi not in ('arm64-v8a', 'mips64', 'x86_64'):
        return abi, 'http://b/26318616'
    return None, None
