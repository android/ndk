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
"""Packages the platform's LLVM for the NDK."""
import argparse
import datetime
import os
import tarfile
import sys


def android_top():
    top = os.getenv('ANDROID_BUILD_TOP', '')
    if not top:
        sys.exit('ANDROID_BUILD_TOP not set. Cannot continue.\n'
                 'Please set ANDROID_BUILD_TOP to point to the root of an '
                 'Android tree.')
    return top


def get_default_package_dir():
    tmp_dir = '/tmp'
    if 'TMPDIR' in os.environ:
        tmp_dir = os.getenv('TMPDIR')
    datestamp = datetime.date.today().isoformat().replace('-', '')
    return os.path.join(tmp_dir, 'prebuilt-{}'.format(datestamp))


def get_llvm_prebuilt_path(host, version):
    rel_prebuilt_path = 'prebuilts/clang/{}/host/{}'.format(
        host, version)
    prebuilt_path = os.path.join(android_top(), rel_prebuilt_path)
    if not os.path.isdir(prebuilt_path):
        sys.exit('Could not find prebuilt LLVM at {}'.format(prebuilt_path))
    return prebuilt_path


class ArgParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgParser, self).__init__(
            description="Package the platform's LLVM for the NDK.")

        self.add_argument(
            '--host', required=True,
            choices=('darwin', 'linux', 'windows'),
            help='Package binaries for given OS (e.g. linux).')
        self.add_argument(
            '--package-dir', help='Directory to place the packaged LLVM.',
            default=get_default_package_dir())


def main():
    LLVM_VERSION = '3.6'

    args = ArgParser().parse_args()
    host = args.host
    package_dir = args.package_dir

    prebuilt_path = get_llvm_prebuilt_path(host + '-x86', LLVM_VERSION)

    if host == 'darwin':
        host = 'darwin-x86_64'
    elif host == 'linux':
        host = 'linux-x86_64'
    elif host == 'windows':
        host = 'windows'

    package_name = 'llvm-{}-{}.tar.bz2'.format(LLVM_VERSION, host)
    package_path = os.path.join(package_dir, package_name)
    with tarfile.TarFile.open(package_path, 'w:bz2') as tarball:
        arcname = 'toolchains/llvm-{}/prebuilt/{}'.format(LLVM_VERSION, host)

        def package_filter(tarinfo):
            if os.path.basename(tarinfo.name) == '.git':
                return None
            if os.path.splitext(tarinfo.name)[1] == '.sh':
                return None
            return tarinfo
        tarball.add(prebuilt_path, arcname, filter=package_filter)

if __name__ == '__main__':
    main()
