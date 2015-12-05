def match_broken(abi, platform, toolchain, subtest=None):
    if toolchain == 'clang':
        return toolchain, 'http://b/26031639'
    return None, None
