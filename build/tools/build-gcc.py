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
"""Packages the platform's GCC for the NDK."""
import os
import shutil
import site
import sys
import tarfile
import tempfile

site.addsitedir(os.path.join(os.path.dirname(__file__), '../lib'))

import build_support  # pylint: disable=import-error


class ArgParser(build_support.ArgParser):
    def __init__(self):  # pylint: disable=super-on-old-class
        super(ArgParser, self).__init__()

        self.add_argument(
            '--arch', choices=build_support.ALL_ARCHITECTURES,
            help='Architecture to build. Builds all if not present.')


def get_gcc_prebuilt_path(host_tag, toolchain_name):
    rel_prebuilt_path = 'prebuilts/ndk/current/toolchains/{}/{}'.format(
        host_tag, toolchain_name)
    prebuilt_path = os.path.join(build_support.android_path(),
                                 rel_prebuilt_path)
    if not os.path.isdir(prebuilt_path):
        sys.exit('Could not find prebuilt GCC at {}'.format(prebuilt_path))
    return prebuilt_path


def package_gcc(package_dir, host_tag, arch, version):
    toolchain_name = build_support.arch_to_toolchain(arch) + '-' + version
    prebuilt_path = get_gcc_prebuilt_path(host_tag, toolchain_name)

    package_name = 'gcc-{}-{}.tar.bz2'.format(arch, host_tag)
    package_path = os.path.join(package_dir, package_name)
    with tarfile.TarFile.open(package_path, 'w:bz2') as tarball:
        arcname = os.path.join('toolchains', host_tag, toolchain_name)

        def package_filter(tarinfo):
            if os.path.basename(tarinfo.name) == '.git':
                return None
            return tarinfo
        tarball.add(prebuilt_path, arcname, filter=package_filter)

        tmpdir = tempfile.mkdtemp()
        try:
            build_support.make_repo_prop(tmpdir)
            arcname = os.path.join(arcname, 'repo.prop')
            tarball.add(os.path.join(tmpdir, 'repo.prop'), arcname)
        finally:
            shutil.rmtree(tmpdir)


def main(args):
    GCC_VERSION = '4.9'

    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    host_tag = build_support.host_to_tag(args.host)
    for arch in arches:
        package_gcc(args.package_dir, host_tag, arch, GCC_VERSION)


if __name__ == '__main__':
    build_support.run(main, ArgParser)
