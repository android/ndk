def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if abi in ('x86', 'x86_64'):
        return abi, 'http://b/24380035'
    return None, None
