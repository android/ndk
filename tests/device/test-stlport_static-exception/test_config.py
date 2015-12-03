def match_broken(abi, platform, device_platform, toolchain, subtest=None):
    # These were marked broken when we inherited them, so there's only one bug
    # for the whole group.
    legacy_broken = (
        'badalloc1',
        'elide2',
        'new3',
        'terminate1',
        'weak1',
    )
    if subtest in legacy_broken:
        return toolchain, 'http://b/24541258'

    if subtest == 'filter1' and device_platform >= 21:
        return toolchain, 'http://b/24403847'

    return None, None
