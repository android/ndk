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
"""Runs all NDK tests.

TODO: Handle default ABI case.
The old test script would build and test all ABIs listed in APP_ABI in a single
invocation if an explicit ABI was not given. Currently this will fall down over
that case. I've written most of the code to handle this, but I decided to
factor that for-each up several levels because it was going to make the device
tests rather messy and I haven't finished doing that yet.

TODO: Handle explicit test lists from command line.
The old test runner allowed specifying an exact list of tests to run with
--tests. That seems like a useful thing to keep around, but I haven't ported it
yet.
"""
from __future__ import print_function

import argparse
import distutils.spawn
import inspect
import os
import re
import shutil
import subprocess
import sys

import adb
import filters
import tests
import util

from tests import AwkTest, BuildTest, DeviceTest


SUPPORTED_ABIS = (
    'armeabi',
    'armeabi-v7a',
    'armeabi-v7a-hard',
    'arm64-v8a',
    'mips',
    'mips64',
    'x86',
    'x86_64',
)


# TODO(danalbert): How much time do we actually save by not running these?
LONG_TESTS = (
    'prebuild-stlport',
    'test-stlport',
    'test-gnustl-full',
    'test-stlport_shared-exception',
    'test-stlport_static-exception',
    'test-gnustl_shared-exception-full',
    'test-gnustl_static-exception-full',
    'test-googletest-full',
    'test-libc++-shared-full',
    'test-libc++-static-full',
)


def get_test_device():
    if distutils.spawn.find_executable('adb') is None:
        raise RuntimeError('Could not find adb.')

    p = subprocess.Popen(['adb', 'devices'], stdout=subprocess.PIPE)
    out, _ = p.communicate()
    if p.returncode != 0:
        raise RuntimeError('Failed to get list of devices from adb.')

    # The first line of `adb devices` just says "List of attached devices", so
    # skip that.
    devices = []
    for line in out.split('\n')[1:]:
        if not line.strip():
            continue
        if 'offline' in line:
            continue

        serial, _ = re.split(r'\s+', line, maxsplit=1)
        devices.append(serial)

    if len(devices) == 0:
        raise RuntimeError('No devices detected.')

    device = os.getenv('ANDROID_SERIAL')
    if device is None and len(devices) == 1:
        device = devices[0]

    if device is not None and device not in devices:
        raise RuntimeError('Device {} is not available.'.format(device))

    # TODO(danalbert): Handle running against multiple devices in one pass.
    if len(devices) > 1 and device is None:
        raise RuntimeError('Multiple devices detected and ANDROID_SERIAL not '
                           'set. Cannot continue.')

    return device


def get_device_abis():
    abis = [adb.get_prop('ro.product.cpu.abi')]
    abi2 = adb.get_prop('ro.product.cpu.abi2')
    if abi2 is not None:
        abis.append(abi2)
    if 'armeabi-v7a' in abis:
        abis.append('armeabi-v7a-hard')
    return abis


def check_adb_works_or_die(abi):
    # TODO(danalbert): Check that we can do anything with the device.
    try:
        device = get_test_device()
    except RuntimeError as ex:
        sys.exit('Error: {}'.format(ex))

    if abi is not None and abi not in get_device_abis():
        sys.exit('The test device ({}) does not support the requested ABI '
                 '({})'.format(device, abi))


def is_valid_platform_version(version_string):
    match = re.match(r'^android-(\d+)$', version_string)
    if not match:
        return False

    # We don't support anything before Gingerbread.
    version = int(match.group(1))
    return version >= 9


def android_platform_version(version_string):
    if is_valid_platform_version(version_string):
        return version_string
    else:
        raise argparse.ArgumentTypeError(
            'Platform version must match the format "android-VERSION", where '
            'VERSION >= 9.')


class ArgParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgParser, self).__init__(
            description=inspect.getdoc(sys.modules[__name__]))

        self.add_argument(
            '--abi', default=None, choices=SUPPORTED_ABIS,
            help=('Run tests against the specified ABI. Defaults to the '
                  'contents of APP_ABI in jni/Application.mk'))
        self.add_argument(
            '--platform', default=None, type=android_platform_version,
            help=('Run tests against the specified platform version. Defaults '
                  'to the contents of APP_PLATFORM in jni/Application.mk'))
        self.add_argument(
            '--show-commands', action='store_true',
            help='Show build commands for each test.')
        self.add_argument(
            '--suite', default=None,
            choices=('awk', 'build', 'device', 'samples'),
            help=('Run only the chosen test suite.'))

        self.add_argument(
            '--filter', help='Only run tests that match the given patterns.')
        self.add_argument(
            '--quick', action='store_true', help='Skip long running tests.')
        self.add_argument(
            '--show-all', action='store_true',
            help='Show all test results, not just failures.')


def main():
    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    # Defining _NDK_TESTING_ALL_=yes to put armeabi-v7a-hard in its own
    # libs/armeabi-v7a-hard directoy and tested separately from armeabi-v7a.
    # Some tests are now compiled with both APP_ABI=armeabi-v7a and
    # APP_ABI=armeabi-v7a-hard. Without _NDK_TESTING_ALL_=yes, tests may fail
    # to install due to race condition on the same libs/armeabi-v7a
    if '_NDK_TESTING_ALL_' not in os.environ:
        os.environ['_NDK_TESTING_ALL_'] = 'all'

    args = ArgParser().parse_args()
    ndk_build_flags = []
    if args.abi is not None:
        ndk_build_flags.append('APP_ABI={}'.format(args.abi))
    if args.platform is not None:
        ndk_build_flags.append('APP_PLATFORM={}'.format(args.platform))
    if args.show_commands:
        ndk_build_flags.append('V=1')

    if not os.path.exists(os.path.join('../build/tools/prebuilt-common.sh')):
        sys.exit('Error: Not run from a valid NDK.')

    out_dir = 'out'
    if os.path.exists(out_dir):
        shutil.rmtree(out_dir)
    os.makedirs(out_dir)

    suites = ['awk', 'build', 'device', 'samples']
    if args.suite:
        suites = [args.suite]

    # Do this early so we find any device issues now rather than after we've
    # run all the build tests.
    if 'device' in suites:
        check_adb_works_or_die(args.abi)
        api_level = int(adb.get_prop('ro.build.version.sdk'))

        # PIE is required in L. All of the device tests are written toward the
        # ndk-build defaults, so we need to inform the build that we need PIE
        # if we're running on a newer device.
        if api_level >= 21:
            ndk_build_flags.append('APP_PIE=true')

    os.environ['ANDROID_SERIAL'] = get_test_device()

    runner = tests.TestRunner()
    if 'awk' in suites:
        runner.add_suite('awk', 'awk', AwkTest)
    if 'build' in suites:
        runner.add_suite('build', 'build', BuildTest, args.abi, args.platform,
                         ndk_build_flags)
    if 'samples' in suites:
        runner.add_suite('samples', '../samples', BuildTest, args.abi,
                         args.platform, ndk_build_flags)
    if 'device' in suites:
        runner.add_suite('device', 'device', DeviceTest, args.abi,
                         args.platform, ndk_build_flags)

    test_filters = filters.TestFilter.from_string(args.filter)
    results = runner.run(out_dir, test_filters)

    num_tests = sum(len(s) for s in results.values())
    zero_stats = {'pass': 0, 'skip': 0, 'fail': 0}
    stats = {suite: dict(zero_stats) for suite in suites}
    global_stats = dict(zero_stats)
    for suite, test_results in results.items():
        for result in test_results:
            if result.failed():
                stats[suite]['fail'] += 1
                global_stats['fail'] += 1
            elif result.passed():
                stats[suite]['pass'] += 1
                global_stats['pass'] += 1
            else:
                stats[suite]['skip'] += 1
                global_stats['skip'] += 1

    def format_stats(num_tests, stats, use_color):
        return '{pl} {p}/{t} {fl} {f}/{t} {sl} {s}/{t}'.format(
            pl=util.color_string('PASS', 'green') if use_color else 'PASS',
            fl=util.color_string('FAIL', 'red') if use_color else 'FAIL',
            sl=util.color_string('SKIP', 'yellow') if use_color else 'SKIP',
            p=stats['pass'], f=stats['fail'],
            s=stats['skip'], t=num_tests)

    use_color = sys.stdin.isatty()
    print()
    print(format_stats(num_tests, global_stats, use_color))
    for suite, test_results in results.items():
        stats_str = format_stats(len(test_results), stats[suite], use_color)
        print()
        print('{}: {}'.format(suite, stats_str))
        for result in test_results:
            if args.show_all or result.failed():
                print(result.to_string(colored=use_color))

    sys.exit(global_stats['fail'] == 0)


if __name__ == '__main__':
    main()
