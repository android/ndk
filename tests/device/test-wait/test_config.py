def match_broken(abi, platform, toolchain, subtest=None):
    if subtest == 'test_wait-static' and abi == 'x86':
        return abi, 'http://b/24507500'
    return None, None
