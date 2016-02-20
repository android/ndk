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
"""
from __future__ import print_function

import argparse
import atexit
import distutils.spawn
import inspect
import os
import re
import shutil
import subprocess
import sys
import tempfile

import adb
import filters
import ndk
import printers
import tests

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
    # 64-bit devices list their ABIs differently than 32-bit devices. Check all
    # the possible places for stashing ABI info and merge them.
    abi_properties = [
        'ro.product.cpu.abi',
        'ro.product.cpu.abi2',
        'ro.product.cpu.abilist',
    ]
    abis = set()
    for abi_prop in abi_properties:
        prop = adb.get_prop(abi_prop)
        if prop is not None:
            abis.update(prop.split(','))

    if 'armeabi-v7a' in abis:
        abis.add('armeabi-v7a-hard')
    return sorted(list(abis))


def check_adb_works_or_die(abi):
    # TODO(danalbert): Check that we can do anything with the device.
    try:
        device = get_test_device()
    except RuntimeError as ex:
        sys.exit('Error: {}'.format(ex))

    supported_abis = get_device_abis()
    if abi is not None and abi not in supported_abis:
        msg = ('The test device ({}) does not support the requested ABI '
               '({}).\nSupported ABIs: {}'.format(device, abi,
                                                  ', '.join(supported_abis)))
        sys.exit(msg)


def can_use_asan(abi, api, toolchain):
    # ASAN is currently only supported for 32-bit ARM and x86...
    if not abi.startswith('armeabi') and not abi == 'x86':
        return False

    # From non-Windows (asan_device_setup is a shell script)...
    if os.name == 'nt':
        return False

    # On KitKat and newer...
    if api < 19:
        return False

    # When using clang...
    if toolchain != 'clang':
        return False

    # On rooted devices.
    if int(adb.get_prop('ro.debuggable')) == 0:
        return False

    return True


def asan_device_setup():
    path = os.path.join(
        os.environ['NDK'], 'toolchains', 'llvm', 'prebuilt',
        ndk.get_host_tag(), 'bin', 'asan_device_setup')
    subprocess.check_call([path])


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
            '--abi', choices=SUPPORTED_ABIS, required=True,
            help=('Run tests against the specified ABI.'))
        self.add_argument(
            '--platform', default=None, type=android_platform_version,
            help=('Run tests against the specified platform version. Defaults '
                  'to the contents of APP_PLATFORM in jni/Application.mk'))
        self.add_argument(
            '--toolchain', default='clang', choices=('4.9', 'clang'),
            help='Toolchain for building tests. Defaults to clang.')

        self.add_argument(
            '--show-commands', action='store_true',
            help='Show build commands for each test.')
        self.add_argument(
            '--suite', default=None,
            choices=('awk', 'build', 'device'),
            help=('Run only the chosen test suite.'))

        self.add_argument(
            '--filter', help='Only run tests that match the given patterns.')
        self.add_argument(
            '--quick', action='store_true', help='Skip long running tests.')
        self.add_argument(
            '--show-all', action='store_true',
            help='Show all test results, not just failures.')

        self.add_argument(
            '--out-dir',
            help='Path to build tests to. Will not be removed upon exit.')


class ResultStats(object):
    def __init__(self, suites, results):
        self.num_tests = sum(len(s) for s in results.values())

        zero_stats = {'pass': 0, 'skip': 0, 'fail': 0}
        self.global_stats = dict(zero_stats)
        self.suite_stats = {suite: dict(zero_stats) for suite in suites}
        self._analyze_results(results)

    def _analyze_results(self, results):
        for suite, test_results in results.items():
            for result in test_results:
                if result.failed():
                    self.suite_stats[suite]['fail'] += 1
                    self.global_stats['fail'] += 1
                elif result.passed():
                    self.suite_stats[suite]['pass'] += 1
                    self.global_stats['pass'] += 1
                else:
                    self.suite_stats[suite]['skip'] += 1
                    self.global_stats['skip'] += 1


def run_all(ndk_path, out_dir, printer, abi, toolchain, build_api_level=None,
            verbose_ndk_build=False, suites=None, test_filter=None):
    """Runs all the tests for the given configuration.

    Sets up the necessary build flags and environment, checks that the device
    under test is in working order and performs device setup (if running device
    tests), and finally runs the tests for the selected suites.

    Args:
        ndk_path: Absolute path the the NDK being tested.
        out_dir: Directory to use when building tests.
        printer: Instance of printers.Printer that will be used to print test
            results.
        abi: ABI to test against.
        toolchain: Toolchain to build with.
        build_api_level: API level to build against. If None, will default to
            the value set in the test's Application.mk, or ndk-build's default.
        verbose_ndk_build: Show verbose output from ndk-build.
        suites: Set of strings denoting which test suites to run. Possible
            values are 'awk', 'build', and 'device'. If None, will run all
            suites.
        test_filter: Filter string for selecting a subset of tests.

    Returns:
        True if all tests completed successfully, False if there were failures.
    """
    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    # Defining _NDK_TESTING_ALL_=yes to put armeabi-v7a-hard in its own
    # libs/armeabi-v7a-hard directory and tested separately from armeabi-v7a.
    # Some tests are now compiled with both APP_ABI=armeabi-v7a and
    # APP_ABI=armeabi-v7a-hard. Without _NDK_TESTING_ALL_=yes, tests may fail
    # to install due to race condition on the same libs/armeabi-v7a
    os.environ['_NDK_TESTING_ALL_'] = 'all'

    os.environ['NDK'] = ndk_path

    # For some reason we handle NDK_TOOLCHAIN_VERSION in _run_ndk_build_test...
    # We also handle APP_ABI there (as well as here). This merits some cleanup.
    ndk_build_flags = ['APP_ABI={}'.format(abi)]
    if build_api_level is not None:
        ndk_build_flags.append('APP_PLATFORM={}'.format(build_api_level))
    if verbose_ndk_build:
        ndk_build_flags.append('V=1')

    # Do this early so we find any device issues now rather than after we've
    # run all the build tests.
    if 'device' in suites:
        check_adb_works_or_die(abi)
        device_api_level = int(adb.get_prop('ro.build.version.sdk'))

        # PIE is required in L. All of the device tests are written toward the
        # ndk-build defaults, so we need to inform the build that we need PIE
        # if we're running on a newer device.
        if device_api_level >= 21:
            ndk_build_flags.append('APP_PIE=true')

        os.environ['ANDROID_SERIAL'] = get_test_device()

        if can_use_asan(abi, device_api_level, toolchain):
            asan_device_setup()

        # Do this as part of initialization rather than with a `mkdir -p` later
        # because Gingerbread didn't actually support -p :(
        adb.shell('rm -r /data/local/tmp/ndk-tests')
        adb.shell('mkdir /data/local/tmp/ndk-tests')

    runner = tests.TestRunner()
    if 'awk' in suites:
        runner.add_suite('awk', 'awk', AwkTest)
    if 'build' in suites:
        runner.add_suite('build', 'build', BuildTest, abi, build_api_level,
                         toolchain, ndk_build_flags)
    if 'device' in suites:
        runner.add_suite('device', 'device', DeviceTest, abi, build_api_level,
                         device_api_level, toolchain, ndk_build_flags)

    test_filters = filters.TestFilter.from_string(test_filter)
    results = runner.run(out_dir, test_filters)

    stats = ResultStats(suites, results)

    printer.print_results(results, stats)
    return stats.global_stats['fail'] == 0


def main():
    orig_cwd = os.getcwd()

    if 'NDK' not in os.environ:
        sys.exit('The environment variable NDK must point to an NDK.')
    ndk_path = os.environ['NDK']

    args = ArgParser().parse_args()

    out_dir = args.out_dir
    if out_dir is not None:
        if not os.path.isabs(out_dir):
            out_dir = os.path.join(orig_cwd, out_dir)
        if os.path.exists(out_dir):
            shutil.rmtree(out_dir)
        os.makedirs(out_dir)
    else:
        out_dir = tempfile.mkdtemp()
        atexit.register(lambda: shutil.rmtree(out_dir))

    suites = ['awk', 'build', 'device']
    if args.suite:
        suites = [args.suite]

    use_color = sys.stdin.isatty() and os.name != 'nt'
    printer = printers.StdoutPrinter(use_color=use_color,
                                     show_all=args.show_all)
    good = run_all(ndk_path, out_dir, printer, args.abi, args.toolchain,
                   args.platform, args.show_commands, suites=suites,
                   test_filter=args.filter)
    sys.exit(not good)


if __name__ == '__main__':
    main()
