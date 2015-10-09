#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Interface to NDK build information."""
import os
import re
import sys

import util


THIS_DIR = os.path.dirname(os.path.realpath(__file__))
NDK_ROOT = os.path.realpath(os.path.join(THIS_DIR, '..'))


def _host_tag():
    if sys.platform.startswith('linux'):
        return 'linux-x86_64'
    elif sys.platform == 'darwin':
        return 'darwin-x86_64'
    elif sys.platform == 'win32':
        test_path = os.path.join(NDK_ROOT, 'prebuilt/windows-x86_64')
        if os.path.exists(test_path):
            return 'windows-x86_64'
        else:
            return 'windows'
    else:
        raise RuntimeError('Unsupported system: {}'.format(sys.platform))


def get_tool(tool):
    ext = ''
    if sys.platform == 'win32':
        ext = '.exe'
    return os.path.join(NDK_ROOT, 'prebuilt', _host_tag(), 'bin', tool) + ext


def build(build_flags):
    ndk_build_path = os.path.join(NDK_ROOT, 'ndk-build')
    return util.call_output([ndk_build_path] + build_flags)


def expand_app_abi(abi):
    all32 = ('armeabi', 'armeabi-v7a', 'armeabi-v7a-hard', 'mips', 'x86')
    all64 = ('arm64-v8a', 'mips64', 'x86_64')
    all_abis = all32 + all64
    if abi == 'all':
        return all_abis
    elif abi == 'all32':
        return all32
    elif abi == 'all64':
        return all64
    return re.split(r'\s+', abi)
