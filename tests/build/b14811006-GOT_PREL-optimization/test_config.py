def match_broken(abi, platform, toolchain, subtest=None):
    if toolchain == 'clang':
        if abi.startswith('armeabi'):
            return ' '.join([toolchain, abi]), 'http://b/26031639'
    return None, None
