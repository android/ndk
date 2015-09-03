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
import collections
import datetime
import inspect
import os
import platform
import site
import subprocess
import sys

site.addsitedir(os.path.join(os.path.dirname(__file__), 'build/lib'))

import build_support


ALL_MODULES = {
    'clang',
    'gcc',
    'gdbserver',
    'gnustl',
    'host-tools',
    'libc++',
    'platforms',
    'stlport',
}


class ArgParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgParser, self).__init__(
            description=inspect.getdoc(sys.modules[__name__]))

        self.add_argument(
            '--arch',
            choices=('arm', 'arm64', 'mips', 'mips64', 'x86', 'x86_64'),
            help='Build for the given architecture. Build all by default.')

        self.add_argument(
            '--package', action='store_true', dest='package', default=True,
            help='Package the NDK when done building.')
        self.add_argument(
            '--no-package', action='store_false', dest='package',
            help='Do not package the NDK when done building.')

        self.add_argument(
            '--release', default=datetime.date.today().strftime('%Y%m%d'),
            help='Release name. Package will be named android-ndk-RELEASE.')

        system_group = self.add_mutually_exclusive_group()
        system_group.add_argument(
            '--system', choices=('darwin', 'linux', 'windows', 'windows64'),
            help='Build for the given OS.')

        old_choices = {
            'darwin', 'darwin-x86',
            'linux', 'linux-x86',
            'windows',
        }

        system_group.add_argument(
            '--systems', choices=old_choices, dest='system',
            help='Build for the given OS. Deprecated. Use --system instead.')

        module_group = self.add_mutually_exclusive_group()

        module_group.add_argument(
            '--module', choices=ALL_MODULES, help='NDK modules to build.')

        module_group.add_argument(
            '--host-only', action='store_true',
            help='Skip building target components.')

        module_group.add_argument(
            '--skip-gcc', action='store_true',
            help='Skip building and packaging GCC.')


def _invoke_build(script, args):
    if args is None:
        args = []
    subprocess.check_call([build_support.android_path(script)] + args)


def invoke_build(script, args=None):
    script_path = os.path.join('build/tools', script)
    _invoke_build(build_support.ndk_path(script_path), args)


def invoke_external_build(script, args=None):
    _invoke_build(build_support.android_path(script), args)


def package_ndk(release_name, system, out_dir, build_args):
    package_args = [
        '--out-dir={}'.format(out_dir),
        '--prebuilt-dir={}'.format(out_dir),
        '--release={}'.format(release_name),
        '--systems={}'.format(system),
    ]
    invoke_build('package-release.sh', package_args + build_args)


def common_build_args(out_dir, args):
    build_args = ['--package-dir={}'.format(out_dir)]
    if args.system is not None:
        # Need to use args.system directly rather than system because system is
        # the name used by the build/tools scripts (i.e. linux-x86 instead of
        # linux).
        build_args.append('--host={}'.format(args.system))
    return build_args


def build_clang(out_dir, args):
    print('Building Clang...')
    invoke_build('build-llvm.py', common_build_args(out_dir, args))


def build_gcc(out_dir, args):
    gcc_build_args = common_build_args(out_dir, args)
    if args.arch is not None:
        toolchain_name = build_support.arch_to_toolchain(args.arch)
        gcc_build_args.append('--toolchain={}'.format(toolchain_name))
    print('Building GCC...')
    invoke_external_build('toolchain/gcc/build.py', gcc_build_args)


def build_host_tools(out_dir, args):
    build_args = common_build_args(out_dir, args)

    print('Building ndk-stack...')
    invoke_external_build(
        'ndk/sources/host-tools/ndk-stack/build.py', build_args)

    print('Building ndk-depends...')
    invoke_external_build(
        'ndk/sources/host-tools/ndk-depends/build.py', build_args)

    print('Building awk...')
    invoke_external_build(
        'ndk/sources/host-tools/nawk-20071023/build.py', build_args)

    print('Building make...')
    invoke_external_build(
        'ndk/sources/host-tools/make-3.81/build.py', build_args)

    if args.system in ('windows', 'windows64'):
        print('Building toolbox...')
        invoke_external_build(
            'ndk/sources/host-tools/toolbox/build.py', build_args)

    print('Building Python...')
    invoke_external_build('toolchain/python/build.py', build_args)

    print('Building GDB...')
    invoke_external_build('toolchain/gdb/build.py', build_args)

    print('Building YASM...')
    invoke_external_build('toolchain/yasm/build.py', build_args)


def build_gdbserver(out_dir, args):
    print('Building gdbserver...')
    build_args = common_build_args(out_dir, args)
    if args.arch is not None:
        build_args.append('--arch={}'.format(args.arch))
    invoke_build('build-gdbserver.py', build_args)


def _build_stl(out_dir, args, stl):
    build_args = common_build_args(out_dir, args)
    if args.arch is not None:
        build_args.append('--arch={}'.format(args.arch))
    script = 'ndk/sources/cxx-stl/{}/build.py'.format(stl)
    invoke_external_build(script, build_args)


def build_gnustl(out_dir, args):
    print('Building gnustl...')
    _build_stl(out_dir, args, 'gnu-libstdc++')


def build_libcxx(out_dir, args):
    print('Building libc++...')
    _build_stl(out_dir, args, 'llvm-libc++')


def build_stlport(out_dir, args):
    print('Building stlport...')
    _build_stl(out_dir, args, 'stlport')


def build_platforms(out_dir, args):
    print('Building platforms...')
    invoke_build('build-platforms.py', common_build_args(out_dir, args))


def main():
    args, package_args = ArgParser().parse_known_args()

    # Disable buffering on stdout so the build output doesn't hide all of our
    # "Building..." messages.
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    # Set ANDROID_BUILD_TOP.
    if 'ANDROID_BUILD_TOP' not in os.environ:
        os.environ['ANDROID_BUILD_TOP'] = os.path.realpath('..')
    build_top = os.getenv('ANDROID_BUILD_TOP')

    system = args.system
    if system != 'windows':
        package_args.append('--try-64')

    if system is not None:
        # TODO(danalbert): Update build server to pass just 'linux'.
        original_system = system
        if system == 'darwin':
            system = 'darwin-x86'
        elif system == 'linux':
            system = 'linux-x86'
        elif system == 'windows64':
            system = 'windows'

        if system not in ('darwin-x86', 'linux-x86', 'windows'):
            sys.exit('Unknown system requested: {}'.format(original_system))

        package_args.append('--systems={}'.format(system))
    else:
        # No flag provided. Use the current OS.
        if platform.system() == 'Darwin':
            system = 'darwin-x86'
        elif platform.system() == 'Linux':
            system = 'linux-x86'
        else:
            sys.exit('Unknown build host: {}'.format(platform.system()))

    package_args.append(os.path.join(build_top, 'toolchain'))

    DEFAULT_OUT_DIR = os.path.join(build_top, 'out/ndk')
    out_dir = os.path.realpath(os.getenv('DIST_DIR', DEFAULT_OUT_DIR))
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    if args.module is None:
        modules = ALL_MODULES
    else:
        modules = {args.module}

    if args.host_only:
        modules = {
            'clang',
            'gcc',
            'host-tools',
        }

    if args.skip_gcc:
        modules = modules - {'gcc'}

    if args.arch is not None:
        package_args.append('--arch={}'.format(args.arch))

    print('Cleaning up...')
    invoke_build('dev-cleanup.sh')

    module_builds = collections.OrderedDict([
        ('clang', build_clang),
        ('gcc', build_gcc),
        ('gdbserver', build_gdbserver),
        ('gnustl', build_gnustl),
        ('host-tools', build_host_tools),
        ('libc++', build_libcxx),
        ('platforms', build_platforms),
        ('stlport', build_stlport),
    ])

    print('Building modules: {}'.format(' '.join(modules)))
    for module in modules:
        module_builds[module](out_dir, args)

    if args.package and modules == ALL_MODULES:
        package_ndk(args.release, system, out_dir, package_args)


if __name__ == '__main__':
    main()
