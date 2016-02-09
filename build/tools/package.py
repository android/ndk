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
import zipfile


site.addsitedir(os.path.join(os.path.dirname(__file__), '../lib'))
import build_support  # pylint: disable=import-error


THIS_DIR = os.path.dirname(__file__)
ANDROID_TOP = os.path.realpath(os.path.join(THIS_DIR, '../../..'))

# Note that we can't actually support creating both layouts from the same
# sources because changes are needed in gnustl's Android.mk and in the build
# system. The various "USE_NEW_LAYOUT" blocks are so we can more easily undo
# this change when we finalize a new layout.
USE_NEW_LAYOUT = False


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

    >>> expand_packages('binutils/{triple}', 'linux', ['arm', 'x86_64'])
    ['binutils/arm-linux-androideabi', 'binutils/x86_64-linux-android']

    >> expand_packages('toolchains/{toolchain}-4.9', 'linux', ['arm', 'x86'])
    ['toolchains/arm-linux-androideabi-4.9', 'toolchains/x86-4.9']
    """
    host_tag = build_support.host_to_tag(host)
    seen_packages = set()
    packages = []
    for arch in arches:
        triple = build_support.arch_to_triple(arch)
        toolchain = build_support.arch_to_toolchain(arch)
        for abi in build_support.arch_to_abis(arch):
            expanded = package.format(
                abi=abi, arch=arch, host=host_tag, triple=triple,
                toolchain=toolchain)
            if expanded not in seen_packages:
                packages.append(expanded)
            seen_packages.add(expanded)
    return packages


def get_all_packages(host, arches):
    new_layout = [
        ('binutils-{arch}-{host}', 'binutils/{triple}'),
        ('build', 'build'),
        ('cpufeatures', 'sources/android/cpufeatures'),
        ('gabixx', 'sources/cxx-stl/gabi++'),
        ('gcc-{arch}-{host}', 'toolchains/{toolchain}-4.9'),
        ('gcclibs-{arch}', 'gcclibs/{triple}'),
        ('gdbserver-{arch}', 'gdbserver/{arch}'),
        ('gnustl-4.9', 'sources/cxx-stl/gnu-libstdc++'),
        ('gtest', 'sources/third_party/googletest'),
        ('host-tools-{host}', 'host-tools'),
        ('libandroid_support', 'sources/android/support'),
        ('libcxx', 'sources/cxx-stl/llvm-libc++'),
        ('libcxxabi', 'sources/cxx-stl/llvm-libc++abi'),
        ('llvm-{host}', 'toolchains/llvm'),
        ('native_app_glue', 'sources/android/native_app_glue'),
        ('ndk_helper', 'sources/android/ndk_helper'),
        ('python-packages', 'python-packages'),
        ('stlport', 'sources/cxx-stl/stlport'),
        ('system-stl', 'sources/cxx-stl/system'),
    ]

    old_layout = [
        ('build', 'build'),
        ('cpufeatures', 'sources/android/cpufeatures'),
        ('gabixx', 'sources/cxx-stl/gabi++'),
        ('gcc-{arch}-{host}', 'toolchains/{toolchain}-4.9/prebuilt/{host}'),
        ('gdbserver-{arch}', 'prebuilt/android-{arch}/gdbserver'),
        ('gnustl-4.9', 'sources/cxx-stl/gnu-libstdc++/4.9'),
        ('gtest', 'sources/third_party/googletest'),
        ('host-tools-{host}', 'prebuilt/{host}'),
        ('libandroid_support', 'sources/android/support'),
        ('libcxx', 'sources/cxx-stl/llvm-libc++'),
        ('libcxxabi', 'sources/cxx-stl/llvm-libc++abi'),
        ('llvm-{host}', 'toolchains/llvm/prebuilt/{host}'),
        ('native_app_glue', 'sources/android/native_app_glue'),
        ('ndk_helper', 'sources/android/ndk_helper'),
        ('python-packages', 'python-packages'),
        ('stlport', 'sources/cxx-stl/stlport'),
        ('system-stl', 'sources/cxx-stl/system'),
    ]

    if USE_NEW_LAYOUT:
        packages = new_layout
    else:
        packages = old_layout

    platforms_path = 'development/ndk/platforms'
    for platform_dir in os.listdir(build_support.android_path(platforms_path)):
        if not platform_dir.startswith('android-'):
            continue
        _, platform_str = platform_dir.split('-')
        package_name = 'platform-' + platform_str
        install_path = 'platforms/android-' + platform_str
        packages.append((package_name, install_path))

    expanded = []
    for package, extract_path in packages:
        package_names = expand_packages(package, host, arches)
        extract_paths = expand_packages(extract_path, host, arches)
        expanded.extend(zip(package_names, extract_paths))
    return expanded


def check_packages(path, packages):
    for package, _ in packages:
        print('Checking ' + package)
        package_path = os.path.join(path, package + '.zip')
        if not os.path.exists(package_path):
            raise RuntimeError('Missing package: ' + package_path)

        top_level_files = []
        with zipfile.ZipFile(package_path, 'r') as zip_file:
            for f in zip_file.namelist():
                components = os.path.split(f)
                if len(components) == 2:
                    top_level_files.append(components[1])

        if 'repo.prop' not in top_level_files:
            msg = 'Package does not contain a repo.prop: ' + package_path
            raise RuntimeError(msg)

        if 'NOTICE' not in top_level_files:
            msg = 'Package does not contain a NOTICE: ' + package_path
            raise RuntimeError(msg)


def extract_all(path, packages, out_dir):
    os.makedirs(out_dir)
    for package, extract_path in packages:
        print('Unpacking ' + package)
        package_path = os.path.join(path, package + '.zip')
        install_dir = os.path.join(out_dir, extract_path)

        if os.path.exists(install_dir):
            raise RuntimeError('Install path already exists: ' + install_dir)

        if extract_path == '.':
            raise RuntimeError('Found old style package: ' + package)

        extract_dir = tempfile.mkdtemp()
        try:
            subprocess.check_call(
                ['unzip', '-q', package_path, '-d', extract_dir])
            dirs = os.listdir(extract_dir)
            if len(dirs) > 1:
                msg = 'Package has more than one root directory: ' + package
                raise RuntimeError(msg)
            elif len(dirs) == 0:
                raise RuntimeError('Package was empty: ' + package)
            parent_dir = os.path.dirname(install_dir)
            if not os.path.exists(parent_dir):
                os.makedirs(parent_dir)
            shutil.move(os.path.join(extract_dir, dirs[0]), install_dir)
        finally:
            shutil.rmtree(extract_dir)

    if not USE_NEW_LAYOUT:
        # FIXME(danalbert): OMG HACK
        # The old package layout had libstdc++'s Android.mk at
        # sources/cxx-stl/gnu-libstdc++/Android.mk. The gnustl package doesn't
        # include the version in the path. To mimic the old package layout, we
        # extract the gnustl package to sources/cxx-stl/gnu-libstdc++/4.9. As
        # such, the Android.mk ends up in the 4.9 directory. We need to pull it
        # up a directory.
        gnustl_path = os.path.join(out_dir, 'sources/cxx-stl/gnu-libstdc++')
        shutil.move(os.path.join(gnustl_path, '4.9/Android.mk'),
                    os.path.join(gnustl_path, 'Android.mk'))


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


def make_source_properties(out_dir):
    path = os.path.join(out_dir, 'source.properties')
    with open(path, 'w') as source_properties:
        source_properties.write('\n'.join([
            'Pkg.Desc = Android NDK',
            'Pkg.Revision = 11.0.0',
        ]))


def copy_changelog(out_dir):
    changelog_path = build_support.ndk_path('CHANGELOG.md')
    shutil.copy2(changelog_path, out_dir)


def make_package(release, package_dir, packages, host, out_dir, temp_dir):
    release_name = 'android-ndk-{}'.format(release)
    extract_dir = os.path.join(temp_dir, release_name)
    if os.path.exists(extract_dir):
        shutil.rmtree(extract_dir)
    extract_all(package_dir, packages, extract_dir)

    make_ndk_build_shortcut(extract_dir, host)
    make_source_properties(extract_dir)
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
            '--dist-dir', type=os.path.realpath,
            default=build_support.get_dist_dir(build_support.get_out_dir()),
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
    check_packages(args.dist_dir, packages)

    if args.unpack:
        extract_all(args.dist_dir, packages, args.out_dir)
        make_ndk_build_shortcut(args.out_dir, args.host)
    else:
        make_package(args.release, args.dist_dir, packages, args.host,
                     args.out_dir, build_support.get_out_dir())


if __name__ == '__main__':
    main()
