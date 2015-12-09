def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    if abi != 'x86':
        return abi

    # mm_malloc.h depends on posix_memalign, which wasn't added until
    # android-16.
    if device_platform < 16:
        return device_platform

    return None
