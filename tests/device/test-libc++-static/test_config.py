def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if subtest == 'test_1_static' and abi == 'mips':
        return abi, 'http://b/24673473'
    return None, None
