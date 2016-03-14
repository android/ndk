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
"""Builds Vulkan components for Android."""
from __future__ import print_function

import os
import shutil
import site
import subprocess
import sys
from string import Template

THIS_DIR = os.path.realpath(os.path.dirname(__file__))

site.addsitedir(os.path.join(THIS_DIR, '../lib'))

import build_support  # pylint: disable=import-error


class ArgParser(build_support.ArgParser):
    def __init__(self):  # pylint: disable=super-on-old-class
        super(ArgParser, self).__init__()

        self.add_argument(
            '--symbols', choices={'true', 'false'},
            help='Include layer symbol files if true, they are large.')
        self.add_argument(
            '--arch', choices=build_support.ALL_ARCHITECTURES,
            help='Architectures to build. Builds all if not present.')

def main(args):
    print('Building Vulkan validation layers...')
    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    abis = []
    for arch in arches:
        abis.extend(build_support.arch_to_abis(arch))

    # According to vk_platform.h, armeabi is not supported for Vulkan
    # so remove it from the abis list.
    abis.remove('armeabi')

    ndk_build = build_support.ndk_path('build/ndk-build')
    vulkan_dir = build_support.android_path('vulkan')
    prebuilt_ndk = build_support.android_path('prebuilts/ndk/current')
    platforms_root = os.path.join(prebuilt_ndk, 'platforms')
    toolchains_root = os.path.join(prebuilt_ndk, 'toolchains')
    vulkan_dir = os.path.join(args.out_dir, 'vulkan')
    build_dir = os.path.join(vulkan_dir, 'buildAndroid')
    package_out = os.path.join(vulkan_dir, 'vulkan')
    includes_out = os.path.join(vulkan_dir, 'include')
    lib_out = os.path.join(package_out, 'libs')
    symbols_out = os.path.join(package_out, 'symbols')

    try:
        os.makedirs(package_out)
    except: pass

    with open(os.path.join(package_out, 'Android.mk'), 'w') as f:
        f.write('LOCAL_PATH:= $(call my-dir)')

    compiler = 'clang'
    stl = 'gnustl_static'
    obj_out = os.path.join(args.out_dir, 'vulkan',stl,'obj')

    build_cmd = [
        'bash', ndk_build, '-C', build_dir, build_support.jobs_arg(),
        'APP_ABI=' + ' '.join(abis),
        # Use the prebuilt platforms and toolchains.
        'NDK_PLATFORMS_ROOT=' + platforms_root,
        'NDK_TOOLCHAINS_ROOT=' + toolchains_root,
        'NDK_NEW_TOOLCHAINS_LAYOUT=true',
        'GNUSTL_PREFIX=',

        # Tell ndk-build where all of our makefiles are and where outputs
        # should go. The defaults in ndk-build are only valid if we have a
        # typical ndk-build layout with a jni/{Android,Application}.mk.
        'NDK_PROJECT_PATH=null',
        'NDK_TOOLCHAIN_VERSION=' + compiler,
        'APP_BUILD_SCRIPT=' + os.path.join(build_dir, 'jni', 'Android.mk'),
        'APP_STL=' + stl,
        'NDK_APPLICATION_MK=' + os.path.join(build_dir, 'jni', 'Application.mk'),
        'NDK_OUT=' + obj_out,
        'NDK_LIBS_OUT=' + lib_out,
        'THIRD_PARTY_PATH=' + build_support.android_path(''),
        # Make sure we don't pick up a cached copy.
        'LIBCXX_FORCE_REBUILD=true',

        # Put armeabi-v7a-hard in its own directory.
        '_NDK_TESTING_ALL_=yes'
    ]

    print('Building Vulkan validation layers for ABIs:' +
          ' {}'.format(', '.join(abis)))
    subprocess.check_call(build_cmd)

    print('Finished building Vulkan validation layers')

    if args.symbols is 'true':
        print('Installing symbol files...')
        copies = [
            {
                'source_dir': os.path.join(build_dir, 'obj'),
                'dest_dir': symbols_out,
                'files': [
                ],
                'dirs': [
                    'local'
                ],
            },
        ]

        default_ignore_patterns = shutil.ignore_patterns(
            "objs")

        for properties in copies:
            source_dir = properties['source_dir']
            dest_dir = os.path.join(package_out, properties['dest_dir'])
            for d in properties['dirs']:
                src = os.path.join(source_dir, d)
                dst = os.path.join(dest_dir, d)
                shutil.rmtree(dst, 'true')
                shutil.copytree(src, dst,
                                ignore=default_ignore_patterns)
            for f in properties['files']:
                install_file(f, source_dir, dest_dir)

    print('Clean intermediate objects...')
    shutil.rmtree(obj_out);
    shutil.rmtree(os.path.join(build_dir, 'libs'))
    shutil.rmtree(os.path.join(build_dir, 'obj'))

if __name__ == '__main__':
    build_support.run(main, ArgParser)
