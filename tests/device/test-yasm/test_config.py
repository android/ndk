def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if abi not in ('x86', 'x86_64'):
        return abi
    return None

def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if abi == 'x86_64':
        return abi, 'http://b/24620865'
    return None, None
