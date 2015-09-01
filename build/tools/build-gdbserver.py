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
"""Builds gdbserver for Android."""
from __future__ import print_function

import os
import site

site.addsitedir(os.path.join(os.path.dirname(__file__), '../lib'))

import build_support


class ArgParser(build_support.ArgParser):
    def __init__(self):
        super(ArgParser, self).__init__()

        self.add_argument(
            '--arch', choices=build_support.ALL_ARCHITECTURES,
            help='Architectures to build. Builds all if not present.')


def main(args):
    GCC_VERSION = '4.9'

    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    print('Building gdbservers: {}'.format(' '.join(arches)))
    for arch in arches:
        toolchain = build_support.arch_to_toolchain(arch)
        toolchain_name = '-'.join([toolchain, GCC_VERSION])
        build_cmd = [
            'bash', 'build-gdbserver.sh', build_support.toolchain_path(),
            build_support.ndk_path(), toolchain_name, build_support.jobs_arg(),
        ]

        build_support.build(build_cmd, args)

if __name__ == '__main__':
    build_support.run(main, ArgParser)
