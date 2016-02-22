def match_unsupported(abi, platform, device_platform, toolchain, subtest=None):
    # Vulkan support wasn't added until android-24
    if device_platform < 24:
        return device_platform

    return None
