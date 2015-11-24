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
import os
import shutil
import site
import sys
import tarfile
import tempfile

site.addsitedir(os.path.join(os.path.dirname(__file__), '../lib'))

import build_support  # pylint: disable=import-error


def get_llvm_prebuilt_path(host, version):
    rel_prebuilt_path = 'prebuilts/clang/host/{}/{}'.format(
        host, version)
    prebuilt_path = os.path.join(build_support.android_path(),
                                 rel_prebuilt_path)
    if not os.path.isdir(prebuilt_path):
        sys.exit('Could not find prebuilt LLVM at {}'.format(prebuilt_path))
    return prebuilt_path


def main(args):
    LLVM_VERSION = 'clang-2445339'

    host = args.host
    package_dir = args.package_dir

    # TODO(danalbert): Fix 64-bit Windows LLVM.
    # This wrongly packages 32-bit Windows LLVM for 64-bit as well, but the
    # real bug here is that we don't have a 64-bit Windows LLVM.
    # http://b/22414702
    os_name = host
    if os_name == 'windows64':
        os_name = 'windows'
    prebuilt_path = get_llvm_prebuilt_path(os_name + '-x86', LLVM_VERSION)

    if host == 'darwin':
        host = 'darwin-x86_64'
    elif host == 'linux':
        host = 'linux-x86_64'
    elif host == 'windows':
        host = 'windows'
    elif host == 'windows64':
        host = 'windows-x86_64'

    package_name = 'llvm-{}.tar.bz2'.format(host)
    package_path = os.path.join(package_dir, package_name)
    with tarfile.TarFile.open(package_path, 'w:bz2') as tarball:
        # TODO(danalbert): Make this version not be a lie. http://b/25782259
        # Right now the version number for clang is in waaaaaaaay to many
        # places to update sanely. We either need to make that not be the case,
        # or just drop the version number entirely.
        #
        # For now, we'll just let the version number continue being a lie (it
        # already was anyway).
        arcname = 'toolchains/llvm-{}'.format('3.6')

        def package_filter(tarinfo):
            if os.path.basename(tarinfo.name) == '.git':
                return None
            if os.path.splitext(tarinfo.name)[1] == '.sh':
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

if __name__ == '__main__':
    build_support.run(main)
