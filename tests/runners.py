#
# Copyright (C) 2016 The Android Open Source Project
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
from __future__ import print_function

import os
import subprocess
import sys

import adb  # pylint: disable=import-error
import filters
import printers
import ndk
import tests
import util

from tests import AwkTest, BuildTest, DeviceTest


def get_device_abis(device):
    # 64-bit devices list their ABIs differently than 32-bit devices. Check all
    # the possible places for stashing ABI info and merge them.
    abi_properties = [
        'ro.product.cpu.abi',
        'ro.product.cpu.abi2',
        'ro.product.cpu.abilist',
    ]
    abis = set()
    properties = device.get_props()
    for abi_prop in abi_properties:
        if abi_prop in properties:
            abis.update(properties[abi_prop].split(','))

    if 'armeabi-v7a' in abis:
        abis.add('armeabi-v7a-hard')
    return sorted(list(abis))


def check_adb_works_or_die(device, abi):
    supported_abis = get_device_abis(device)
    if abi is not None and abi not in supported_abis:
        msg = ('The test device ({}) does not support the requested ABI '
               '({}).\nSupported ABIs: {}'.format(device.serial, abi,
                                                  ', '.join(supported_abis)))
        sys.exit(msg)


def can_use_asan(device, abi, api, toolchain):
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
    if int(device.get_prop('ro.debuggable')) == 0:
        return False

    return True


def asan_device_setup():
    path = os.path.join(
        os.environ['NDK'], 'toolchains', 'llvm', 'prebuilt',
        ndk.get_host_tag(), 'bin', 'asan_device_setup')
    try:
        # Don't want to use check_call because we want to keep this quiet
        # unless there's a problem.
        subprocess.check_output([path], stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as ex:
        print(ex.output)
        raise ex


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


def run_single_configuration(ndk_path, out_dir, printer, abi, toolchain,
                             build_api_level=None, verbose_ndk_build=False,
                             suites=None, test_filter=None,
                             device_serial=None):
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
        device_serial: Serial number of the device to use for device tests. If
            none, will try to find a device from ANDROID_SERIAL or a unique
            attached device.

    Returns:
        True if all tests completed successfully, False if there were failures.
    """
    if suites is None:
        suites = ('awk', 'build', 'device')

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
        device = adb.get_device(device_serial)
        check_adb_works_or_die(device, abi)
        device_api_level = int(device.get_prop('ro.build.version.sdk'))

        # PIE is required in L. All of the device tests are written toward the
        # ndk-build defaults, so we need to inform the build that we need PIE
        # if we're running on a newer device.
        if device_api_level >= 21:
            ndk_build_flags.append('APP_PIE=true')

        os.environ['ANDROID_SERIAL'] = device.serial

        if can_use_asan(device, abi, device_api_level, toolchain):
            asan_device_setup()

        # Do this as part of initialization rather than with a `mkdir -p` later
        # because Gingerbread didn't actually support -p :(
        device.shell_nocheck(['rm -r /data/local/tmp/ndk-tests 2>&1'])
        device.shell(['mkdir /data/local/tmp/ndk-tests 2>&1'])

    runner = tests.TestRunner(printer)
    if 'awk' in suites:
        runner.add_suite('awk', 'awk', AwkTest)
    if 'build' in suites:
        runner.add_suite('build', 'build', BuildTest, abi, build_api_level,
                         toolchain, ndk_build_flags)
    if 'device' in suites:
        runner.add_suite('device', 'device', DeviceTest, abi, build_api_level,
                         device, device_api_level, toolchain, ndk_build_flags)

    test_filters = filters.TestFilter.from_string(test_filter)
    results = runner.run(out_dir, test_filters)

    stats = ResultStats(suites, results)

    printer.print_summary(results, stats)
    return stats.global_stats['fail'] == 0, results


def run_tests(ndk_path, device, abi, toolchain, out_dir, log_dir):
    print('Running {} {} tests for {}... '.format(toolchain, abi, device),
          end='')
    sys.stdout.flush()

    toolchain_name = 'gcc' if toolchain == '4.9' else toolchain
    log_file_name = '{}-{}-{}.log'.format(toolchain_name, abi, device.version)
    with open(os.path.join(log_dir, log_file_name), 'w') as log_file:
        printer = printers.FilePrinter(log_file)
        good, details = run_single_configuration(
            ndk_path, out_dir, printer, abi, toolchain,
            device_serial=device.serial)
        print('PASS' if good else 'FAIL')
        return good, details


def run_for_fleet(ndk_path, fleet, out_dir, log_dir, use_color=False):
    # Note that we are duplicating some testing here.
    #
    # * The awk tests only need to be run once because they do not vary by
    #   configuration.
    # * The build tests only vary per-device by the PIE configuration, so we
    #   only need to run them twice per ABI/toolchain.
    # * The build tests are already run as a part of the build process.
    results = []
    details = {}
    good = True
    for version in fleet.get_versions():
        details[version] = {}
        for abi in fleet.get_abis(version):
            details[version][abi] = {}
            device = fleet.get_device(version, abi)
            for toolchain in ('clang', '4.9'):
                if device is None:
                    results.append('android-{} {} {}: {}'.format(
                        version, abi, toolchain, 'SKIP'))
                    continue

                details[version][abi][toolchain] = None

                result, run_details = run_tests(
                    ndk_path, device, abi, toolchain, out_dir, log_dir)
                pass_label = util.maybe_color('PASS', 'green', use_color)
                fail_label = util.maybe_color('FAIL', 'red', use_color)
                results.append('android-{} {} {}: {}'.format(
                    version, abi, toolchain,
                    pass_label if result else fail_label))
                details[version][abi][toolchain] = run_details
                if not result:
                    good = False

    print('\n'.join(results))
    return good, details
