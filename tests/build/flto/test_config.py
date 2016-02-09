# Shut up a warning about us not being a real package.
from __future__ import absolute_import
import platform


def match_unsupported(abi, _platform, toolchain, _subtest=None):
    # Clang does LTO via gold plugin, but gold doesn't support MIPS yet.
    if toolchain == 'clang' and abi.startswith('mips'):
        return '{} {}'.format(toolchain, abi)

    # We only support LTO from Linux.
    if platform.system() != 'Linux':
        return platform.system()

    return None
