def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    if abi.startswith('armeabi') and device_platform <= 10:
        return '{} {}'.format(abi, device_platform), 'http://b/28044141'
    return None, None
