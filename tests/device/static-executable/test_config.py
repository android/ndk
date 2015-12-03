def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    lp64_abis = ('arm64-v8a', 'mips64', 'x86_64')
    if platform is not None:
        platform_version = int(platform.split('-')[1])
        if platform_version >= 21:
            return platform, 'http://b/24468267'
    elif abi in lp64_abis:
        # If platform isn't provided we're going to use the default platform
        # level. For 32-bit ABIs the default is old enough that we'll pass, but
        # 64-bit ABIs will fail.
        return 'android-21', 'http://b/24468267'
    return None, None
