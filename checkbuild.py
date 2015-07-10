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
"""Verifies that the build is sane.

Cleans old build artifacts, configures the required environment, determines
build goals, and invokes the build scripts.
"""
import argparse
import inspect
import os
import platform
import subprocess
import sys


class ArgParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgParser, self).__init__(
            description=inspect.getdoc(sys.modules[__name__]))

        system_group = self.add_mutually_exclusive_group()
        system_group.add_argument(
            '--system', choices=('darwin', 'linux', 'windows'),
            help='Build for the given OS.')

        old_choices = (
            'darwin', 'darwin-x86',
            'linux', 'linux-x86',
            'windows',
        )

        system_group.add_argument(
            '--systems', choices=old_choices, dest='system',
            help='Build for the given OS. Deprecated. Use --system instead.')


def invoke_build(script, args=None):
    if args is None:
        args = []
    subprocess.check_call([os.path.join('build/tools', script)] + args)


def main():
    args, build_args = ArgParser().parse_known_args()
    build_args.append('--verbose')

    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    # Set ANDROID_BUILD_TOP.
    if 'ANDROID_BUILD_TOP' not in os.environ:
        os.environ['ANDROID_BUILD_TOP'] = os.path.realpath('..')
    build_top = os.getenv('ANDROID_BUILD_TOP')

    # Set default --package-dir
    DEFAULT_OUT_DIR = os.path.join(build_top, 'out/ndk')
    package_dir = os.path.realpath(os.getenv('DIST_DIR', DEFAULT_OUT_DIR))
    build_args.append('--package-dir={}'.format(package_dir))

    system = args.system
    if system is not None:
        # TODO(danalbert): Update build server to pass just 'linux'.
        original_system = system
        if system == 'darwin':
            system = 'darwin-x86'
        elif system == 'linux':
            system = 'linux-x86'

        if system not in ('darwin-x86', 'linux-x86', 'windows'):
            sys.exit('Unknown system requested: {}'.format(original_system))

        build_args.append('--systems={}'.format(system))

    if system != 'windows':
        build_args.append('--try-64')

    build_args.append(os.path.join(build_top, 'toolchain'))

    # Run dev-cleanup
    invoke_build('dev-cleanup.sh')

    # Build
    if system == 'windows' or platform.system() == 'Darwin':
        # There's no sense in building the target libraries from Darwin (they
        # shouldn't be any different from those built on Linux), and we can't
        # build them using the Windows toolchains (because we aren't on
        # Windows), so only build the host components.
        ndk_dir_arg = '--ndk-dir={}'.format(os.getcwd())
        invoke_build('build-host-prebuilts.sh',
                     build_args + [ndk_dir_arg])
    else:
        invoke_build('rebuild-all-prebuilt.sh', build_args)


if __name__ == '__main__':
    main()
