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
import subprocess


THIS_DIR = os.path.dirname(os.path.realpath(__file__))
NDK_ROOT = os.path.realpath(os.path.join(THIS_DIR, '..'))


def get_build_var(test_dir, var_name):
    makefile = os.path.join(NDK_ROOT, 'build/core/build-local.mk')
    cmd = ['make', '--no-print-dir', '-f', makefile, '-C', test_dir,
           'DUMP_{}'.format(var_name)]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    out, _ = p.communicate()
    if p.returncode != 0:
        raise RuntimeError('Could not get build variable')
    return out.strip().split('\n')[-1]


def build(build_flags):
    ndk_build_path = os.path.join(NDK_ROOT, 'ndk-build')
    return subprocess.call([ndk_build_path] + build_flags)


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
