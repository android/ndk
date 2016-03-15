def match_unsupported(abi, platform, toolchain, subtest=None):
    # Note that these test that use .arm and .neon actually do work for
    # x86/x86_64 because those ABIs have some emulation for neon, and ignore
    # the .arm tag.
    #
    # While armeabi doesn't support .neon, it doesn't support .arm, so we
    # disable that test in the Android.mk.
    if abi in ('mips', 'mips64'):
        return abi
    return None
