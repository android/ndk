def match_unsupported(abi, platform, toolchain, subtest=None):
    if abi == 'mips':
        return abi, 'http://b.android.com/41297'
    return None
