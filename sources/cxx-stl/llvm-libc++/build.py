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
"""Builds libc++ for Android."""
from __future__ import print_function

import os
import shutil
import site
import subprocess

THIS_DIR = os.path.realpath(os.path.dirname(__file__))
site.addsitedir(os.path.join(THIS_DIR, '../../../build/lib'))

import build_support  # pylint: disable=import-error


class ArgParser(build_support.ArgParser):
    def __init__(self):  # pylint: disable=super-on-old-class
        super(ArgParser, self).__init__()

        self.add_argument(
            '--arch', choices=build_support.ALL_ARCHITECTURES,
            help='Architectures to build. Builds all if not present.')


def main(args):
    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    abis = []
    for arch in arches:
        abis.extend(build_support.arch_to_abis(arch))

    ndk_build = build_support.ndk_path('build/ndk-build')
    prebuilt_ndk = build_support.android_path('prebuilts/ndk/current')
    platforms_root = os.path.join(prebuilt_ndk, 'platforms')
    toolchains_root = os.path.join(prebuilt_ndk, 'toolchains')
    libcxx_path = build_support.ndk_path('sources/cxx-stl/llvm-libc++')
    obj_out = os.path.join(args.out_dir, 'libcxx/obj')

    # TODO(danalbert): Stop building to the source directory.
    # This is historical, and simplifies packaging a bit. We need to pack up
    # all the source as well as the libraries. If build_support.make_package
    # were to change to allow a list of directories instead of one directory,
    # we could make this unnecessary.  Will be a follow up CL.
    lib_out = os.path.join(libcxx_path, 'libs')

    build_cmd = [
        'bash', ndk_build, '-C', THIS_DIR, build_support.jobs_arg(), 'V=1',
        'APP_ABI=' + ' '.join(abis),

        # Use the prebuilt platforms and toolchains.
        'NDK_PLATFORMS_ROOT=' + platforms_root,
        'NDK_TOOLCHAINS_ROOT=' + toolchains_root,
        'NDK_NEW_TOOLCHAINS_LAYOUT=true',

        # Tell ndk-build where all of our makefiles are and where outputs
        # should go. The defaults in ndk-build are only valid if we have a
        # typical ndk-build layout with a jni/{Android,Application}.mk.
        'NDK_PROJECT_PATH=null',
        'APP_BUILD_SCRIPT=' + os.path.join(THIS_DIR, 'Android.mk'),
        'NDK_APPLICATION_MK=' + os.path.join(THIS_DIR, 'Application.mk'),
        'NDK_OUT=' + obj_out,
        'NDK_LIBS_OUT=' + lib_out,

        # Make sure we don't pick up a cached copy.
        'LIBCXX_FORCE_REBUILD=true',

        # Put armeabi-v7a-hard in its own directory.
        '_NDK_TESTING_ALL_=yes',
    ]
    print('Building libc++ for ABIs: {}'.format(', '.join(abis)))
    subprocess.check_call(build_cmd)

    # The static libraries are installed to NDK_OUT, not NDK_LIB_OUT, so we
    # need to install them to our package directory.
    for abi in abis:
        static_lib_dir = os.path.join(obj_out, 'local', abi)
        install_dir = os.path.join(lib_out, abi)
        is_arm = abi.startswith('armeabi')

        if is_arm:
            shutil.copy2(
                os.path.join(static_lib_dir, 'libunwind.a'), install_dir)

        shutil.copy2(os.path.join(static_lib_dir, 'libc++abi.a'), install_dir)
        shutil.copy2(
            os.path.join(static_lib_dir, 'libandroid_support.a'), install_dir)
        shutil.copy2(
            os.path.join(static_lib_dir, 'libc++_static.a'), install_dir)

    build_support.make_package('libcxx', libcxx_path, args.dist_dir)


if __name__ == '__main__':
    build_support.run(main, ArgParser)
