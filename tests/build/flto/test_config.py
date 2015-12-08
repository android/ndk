def match_unsupported(abi, platform, toolchain, subtest=None):
    # Clang does LTO via gold plugin, but gold doesn't support MIPS yet.
    if toolchain == 'clang' and abi.startswith('mips'):
        return '{} {}'.format(toolchain, abi)
