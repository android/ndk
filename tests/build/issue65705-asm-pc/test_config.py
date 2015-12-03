def match_broken(abi, device, toolchain, subtest=None):
    if toolchain == '4.9':
        return toolchain, 'http://b.android.com/65705'
    return None, None


def match_unsupported(abi, device, toolchain, subtest=None):
    if abi not in ('armeabi', 'armeabi-v7a', 'armeabi-v7a-hard'):
        return abi
    return None
