def match_broken(abi, platform, toolchain, subtest=None):
    if toolchain == 'clang' and abi.startswith('armeabi-v7a'):
        return '{} {}'.format(toolchain, abi), 'http://b/26091410'
    return None, None
