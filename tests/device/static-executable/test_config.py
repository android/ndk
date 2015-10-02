def match_broken(abi, platform, toolchain, subtest=None):
    platform_version = int(platform.split('-')[1])
    if platform_version >= 21:
        return platform, 'http://b/24468267'
    return None, None
