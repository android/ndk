def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if toolchain == 'clang':
        return toolchain, 'http://b/25937032'
    return None, None
