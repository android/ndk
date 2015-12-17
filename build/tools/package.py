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
"""Makes an old style monolithic NDK package out of individual modules."""
from __future__ import print_function

import argparse
import datetime
import os
import shutil
import site
import stat
import subprocess
import sys
import tempfile


site.addsitedir(os.path.join(os.path.dirname(__file__), '../lib'))
import build_support  # pylint: disable=import-error


THIS_DIR = os.path.dirname(__file__)
ANDROID_TOP = os.path.realpath(os.path.join(THIS_DIR, '../../..'))


def expand_packages(package, host, arches):
    """Expands package definition tuple into list of full package names.

    >>> expand_packages('gcc-{arch}-{host}', 'linux', ['arm64', 'x86_64'])
    ['gcc-arm64-linux-x86_64', 'gcc-x86_64-linux-x86_64']

    >>> expand_packages('gcclibs-{arch}', 'linux', ['arm64', 'x86_64'])
    ['gcclibs-arm64', 'gcclibs-x86_64']

    >>> expand_packages('llvm-{host}', 'linux', ['arm'])
    ['llvm-linux-x86_64']

    >>> expand_packages('platforms', 'linux', ['arm'])
    ['platforms']

    >>> expand_packages('libc++-{abi}', 'linux', ['arm'])
    ['libc++-armeabi', 'libc++-armeabi-v7a', 'libc++-armeabi-v7a-hard']

    >>> expand_packages('{triple}-4.9-{host}', 'linux', ['arm', 'x86_64'])
    ['arm-linux-androideabi-4.9-linux-x86_64', 'x86_64-4.9-linux-x86_64']
    """
    host_tag = build_support.host_to_tag(host)
    packages = set()
    for arch in arches:
        triple = build_support.arch_to_toolchain(arch)
        for abi in build_support.arch_to_abis(arch):
            packages.add(package.format(
                abi=abi, arch=arch, host=host_tag, triple=triple))

    # Sorting isn't really necessary; it's just for the benefit of the
    # doctests. These are small sets, so it's basically free.
    return sorted(packages)


def get_all_packages(host, arches):
    packages = [
        'binutils-{arch}-{host}',
        'build',
        'cpufeatures',
        'gcc-{arch}-{host}',
        'gcclibs-{arch}',
        'gdbserver-{arch}',
        'gnustl-4.9',
        'gtest',
        'host-tools-{host}',
        'libcxx',
        'llvm-{host}',
        'native_app_glue',
        'ndk_helper',
        'python-packages',
        'stlport',
    ]

    platforms_path = 'development/ndk/platforms'
    for platform_dir in os.listdir(build_support.android_path(platforms_path)):
        if not platform_dir.startswith('android-'):
            continue
        _, platform_str = platform_dir.split('-')
        packages.append('platform-' + platform_str)

    expanded = []
    for package in packages:
        expanded.extend(expand_packages(package, host, arches))
    return expanded


def check_packages(path, packages):
    for package in packages:
        print('Checking ' + package)
        package_path = os.path.join(path, package + '.tar.bz2')
        if not os.path.exists(package_path):
            raise RuntimeError('Missing package: ' + package_path)
        output = subprocess.check_output(['tar', 'tf', package_path])
        for f in output.splitlines():
            file_name = os.path.basename(f)
            if file_name == 'repo.prop':
                break
        else:
            msg = 'Package does not contain a repo.prop: ' + package_path
            raise RuntimeError(msg)


def extract_all(path, packages, out_dir):
    os.makedirs(out_dir)
    for package in packages:
        print('Unpacking ' + package)
        package_path = os.path.join(path, package + '.tar.bz2')
        subprocess.check_call(['tar', 'xf', package_path, '-C', out_dir])


def make_ndk_build_shortcut(out_dir, host):
    if host.startswith('windows'):
        make_ndk_build_cmd_helper(out_dir)
    else:
        make_ndk_build_sh_helper(out_dir)


def make_ndk_build_cmd_helper(out_dir):
    with open(os.path.join(out_dir, 'ndk-build.cmd'), 'w') as helper:
        helper.writelines([
            '@echo off\n',
            r'%~dp0\build\ndk-build.cmd %*',
        ])


def make_ndk_build_sh_helper(out_dir):
    file_path = os.path.join(out_dir, 'ndk-build')
    with open(file_path, 'w') as helper:
        helper.writelines([
            '#!/bin/sh\n',
            'DIR="$(cd "$(dirname "$0")" && pwd)"\n',
            '$DIR/build/ndk-build "$@"',
        ])
    mode = os.stat(file_path).st_mode
    os.chmod(file_path, mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def copy_changelog(out_dir):
    changelog_path = build_support.ndk_path('CHANGELOG.md')
    shutil.copy2(changelog_path, out_dir)


def make_package(release, package_dir, packages, host, out_dir, temp_dir):
    release_name = 'android-ndk-{}'.format(release)
    extract_dir = os.path.join(temp_dir, release_name)
    extract_all(package_dir, packages, extract_dir)

    make_ndk_build_shortcut(extract_dir, host)
    copy_changelog(extract_dir)

    host_tag = build_support.host_to_tag(host)
    package_name = '{}-{}'.format(release_name, host_tag)
    package_path = os.path.join(out_dir, package_name)
    print('Packaging ' + package_name)
    files = os.path.relpath(extract_dir, temp_dir)

    if host.startswith('windows'):
        _make_zip_package(package_path, temp_dir, files)
    else:
        _make_tar_package(package_path, temp_dir, files)


def _make_tar_package(package_path, base_dir, files):
    subprocess.check_call(
        ['tar', 'cjf', package_path + '.tar.bz2', '-C', base_dir, files])


def _make_zip_package(package_path, base_dir, files):
    cwd = os.getcwd()
    package_path = os.path.realpath(package_path)
    os.chdir(base_dir)
    try:
        subprocess.check_call(['zip', '-9qr', package_path + '.zip', files])
    finally:
        os.chdir(cwd)


class ArgParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgParser, self).__init__(
            description='Repackages NDK modules as a monolithic package. If '
                        '--unpack is used, instead installs the monolithic '
                        'package to a directory.')

        self.add_argument(
            '--arch', choices=build_support.ALL_ARCHITECTURES,
            help='Bundle only the given architecture.')
        self.add_argument(
            '--host', choices=('darwin', 'linux', 'windows', 'windows64'),
            default=build_support.get_default_host(),
            help='Package binaries for given OS (e.g. linux).')
        self.add_argument(
            '--release', default=datetime.date.today().strftime('%Y%m%d'),
            help='Release name for the package.')

        self.add_argument(
            '-f', '--force', dest='force', action='store_true',
            help='Clobber out directory if it exists.')
        self.add_argument(
            '--package-dir', type=os.path.realpath,
            default=build_support.get_default_package_dir(),
            help='Directory containing NDK modules.')
        self.add_argument(
            '--unpack', action='store_true',
            help='Unpack the NDK to a directory rather than make a tarball.')
        self.add_argument(
            'out_dir', metavar='OUT_DIR',
            help='Directory to install bundle or tarball to.')


def main():
    if 'ANDROID_BUILD_TOP' not in os.environ:
        os.environ['ANDROID_BUILD_TOP'] = ANDROID_TOP

    args = ArgParser().parse_args()
    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    if os.path.exists(args.out_dir) and args.unpack:
        if args.force:
            shutil.rmtree(args.out_dir)
        else:
            sys.exit(args.out_dir + ' already exists. Use -f to overwrite.')

    packages = get_all_packages(args.host, arches)
    check_packages(args.package_dir, packages)

    if args.unpack:
        extract_all(args.package_dir, packages, args.out_dir)
        make_ndk_build_shortcut(args.out_dir, args.host)
    else:
        package_dir = tempfile.mkdtemp()
        try:
            make_package(args.release, args.package_dir, packages, args.host,
                         args.out_dir, package_dir)
        finally:
            shutil.rmtree(package_dir)


if __name__ == '__main__':
    main()
