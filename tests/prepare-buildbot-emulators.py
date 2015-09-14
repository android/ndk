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
"""Installs an SDK and creates emulators for buildbot testing."""
from __future__ import print_function

import os
import platform
import shutil
import subprocess


DEVICES = {
    '10': ['x86'],
    '16': ['armeabi-v7a', 'mips', 'x86'],
    '23': ['armeabi-v7a', 'x86', 'x86_64'],
}


def get_sdk():
    os_name = platform.system().lower()
    ext = 'tgz'
    if os_name == 'darwin':
        os_name = 'macosx'
        ext = 'zip'

    url = 'http://dl.google.com/android/android-sdk_r24.3.4-{}.{}'.format(
        os_name, ext)

    package_name = os.path.basename(url)
    if os.path.exists(package_name):
        os.remove(package_name)

    sdk_dir = 'android-sdk-' + os_name
    if os.path.exists(sdk_dir):
        shutil.rmtree(sdk_dir)

    subprocess.check_call(['curl', '-O', url])
    if ext == 'tgz':
        subprocess.check_call(['tar', 'xf', package_name])
    else:
        subprocess.check_call(['unzip', package_name])

    if os.path.exists(package_name):
        os.remove(package_name)

    return sdk_dir


def install_components(sdk_manager):
    packages = [
        'platform-tools',
    ]

    for api, abis in DEVICES.items():
        packages.append('android-' + api)
        for abi in abis:
            packages.append('sys-img-{}-android-{}'.format(abi, api))

    print('Installing packages:')
    print('\n'.join(packages))

    filter_arg = ','.join(packages)

    cmd = [
        sdk_manager, 'update', 'sdk', '--no-ui', '--all', '--filter',
        filter_arg,
    ]
    subprocess.check_call(cmd)


def create_devices(sdk_manager):
    for api, abis in DEVICES.items():
        for abi in abis:
            avd_name = '-'.join([abi, api])
            api_name = 'android-' + api

            print('Creating AVD for {}'.format(avd_name))

            cmd = [
                sdk_manager, 'create', 'avd', '--force', '--name', avd_name,
                '--target', api_name, '--abi', abi,
            ]
            proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
            proc.communicate('no\n')  # No custom hardware profile.


def main():
    os.chdir(os.getenv('HOME'))

    sdk_dir = get_sdk()
    sdk_manager = os.path.join(sdk_dir, 'tools/android')
    install_components(sdk_manager)
    create_devices(sdk_manager)


if __name__ == '__main__':
    main()
