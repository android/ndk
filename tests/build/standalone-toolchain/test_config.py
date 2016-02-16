# Shut up a warning about us not being a real package.
from __future__ import absolute_import
import platform


def match_unsupported(_abi, _api_level, _toolchain, _subtest=None):
    if platform.system() == 'Windows':
        # make-standalone-toolchain.sh isn't supported on Windows since we
        # don't have bash. Note that platform.system() won't report "Windows"
        # for cygwin, so we will run the test in that case.
        return platform.system()
    return None
