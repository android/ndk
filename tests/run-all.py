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
import contextlib
import distutils.spawn
import functools
import glob
import inspect
import multiprocessing
import os
import re
import shutil
import subprocess
import sys

import adb
import ndk


DEV_NULL = open(os.devnull, 'wb')


SUPPORTED_ABIS = (
    'armeabi',
    'armeabi-v7a',
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


@contextlib.contextmanager
def cd(path):
    curdir = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(curdir)


def get_jobs_arg():
    return '-j{}'.format(multiprocessing.cpu_count() * 2)


def color_string(string, color):
    colors = {
        'green': '\033[92m',
        'red': '\033[91m',
        'yellow': '\033[93m',
    }
    end_color = '\033[0m'
    return colors[color] + string + end_color


class TestResult(object):
    def __init__(self, test_name, passed):
        self.passed = passed
        self.test_name = test_name

    def __repr__(self):
        return self.to_string(colored=False)

    def to_string(self, colored=False):
        raise NotImplementedError


class Failure(TestResult):
    def __init__(self, test_name, message):
        super(Failure, self).__init__(test_name, passed=False)
        self.message = message

    def to_string(self, colored=False):
        label = color_string('FAIL', 'red') if colored else 'FAIL'
        return '{} {}: {}'.format(label, self.test_name, self.message)


class Success(TestResult):
    def __init__(self, test_name):
        super(Success, self).__init__(test_name, passed=True)

    def to_string(self, colored=False):
        label = color_string('PASS', 'green') if colored else 'PASS'
        return '{} {}'.format(label, self.test_name)


class Skipped(TestResult):
    def __init__(self, test_name, reason):
        super(Skipped, self).__init__(test_name, passed=False)
        self.reason = reason

    def to_string(self, colored=False):
        label = color_string('SKIP', 'yellow') if colored else 'SKIP'
        return '{} {}: {}'.format(label, self.test_name, self.reason)


def test_is_disabled(test_dir, platform):
    disable_file = os.path.join(test_dir, 'BROKEN_BUILD')
    if os.path.isfile(disable_file):
        if os.stat(disable_file).st_size == 0:
            return True

        # This might look like clang-3.6 and gcc-3.6 would overlap (not a
        # problem today, but maybe when we hit clang-4.9), but clang is
        # actually written as clang3.6 (with no hypen), so toolchain_version
        # will end up being 'clang3.6'.
        toolchain = ndk.get_build_var(test_dir, 'TARGET_TOOLCHAIN')
        toolchain_version = toolchain.split('-')[-1]
        with open(disable_file) as f:
            contents = f.read()
        broken_configs = re.split(r'\s+', contents)
        if toolchain_version in broken_configs:
            return True
        if platform is not None and platform in broken_configs:
            return True
    return False


def run_is_disabled(test_case, test_dir):
    """Returns True if the test case is disabled.

    There is no strict format for the BROKEN_RUN file; test cases are disabled
    if their basename appears anywhere in the file.
    """
    disable_file = os.path.join(test_dir, 'BROKEN_RUN')
    if not os.path.exists(disable_file):
        return False
    return subprocess.call(['grep', '-qw', test_case, disable_file]) == 0


def should_skip_test(test_dir, abi, platform):
    if test_is_disabled(test_dir, platform):
        return 'disabled'
    if abi is not None:
        app_abi = ndk.get_build_var(test_dir, 'APP_ABI')
        supported_abis = ndk.expand_app_abi(app_abi)
        if abi not in supported_abis:
            abi_string = ', '.join(supported_abis)
            return 'incompatible ABI (requires {})'.format(abi_string)
    return None


def run_awk_test_case(out_dir, test_name, script, test_case, golden_out_path):
    out_path = os.path.join(out_dir, os.path.basename(golden_out_path))

    with open(test_case, 'r') as test_in, open(out_path, 'w') as out_file:
        print('awk -f {} < {} > {}'.format(script, test_case, out_path))
        rc = subprocess.call(['awk', '-f', script], stdin=test_in,
                             stdout=out_file)
        if rc != 0:
            return Failure(test_name, 'awk failed')

    rc = subprocess.call(['cmp', out_path, golden_out_path], stdout=DEV_NULL,
                         stderr=DEV_NULL)
    if rc == 0:
        return Success(test_name)
    else:
        p = subprocess.Popen(['diff', '-buN', out_path, golden_out_path],
                             stdout=subprocess.PIPE, stderr=DEV_NULL)
        out, _ = p.communicate()
        if p.returncode != 0:
            raise RuntimeError('Could not generate diff')
        message = 'output does not match expected:\n\n' + out
        return Failure(test_name, message)


def run_awk_test(out_dir, test_dir):
    test_name = '{}.awk'.format(os.path.basename(test_dir))
    script = os.path.join(ndk.NDK_ROOT, 'build/awk', test_name)
    if not os.path.isfile(script):
        return [Failure(test_name, 'missing test script: {}'.format(script))]
    results = []
    for test_case in glob.glob(os.path.join(test_dir, '*.in')):
        golden_path = re.sub(r'\.in$', '.out', test_case)
        if not os.path.isfile(golden_path):
            results.append(Failure(test_name,
                                   'missing output: {}'.format(golden_path)))
        results.append(run_awk_test_case(out_dir, test_name, script, test_case,
                                         golden_path))
    return results


def prep_build_dir(src_dir, out_dir):
    if os.path.exists(out_dir):
        shutil.rmtree(out_dir)
    shutil.copytree(src_dir, out_dir)


def run_build_sh_test(test_name, build_dir, test_dir, build_flags, abi,
                      platform):
    android_mk = os.path.join(test_dir, 'jni/Android.mk')
    application_mk = os.path.join(test_dir, 'jni/Application.mk')
    if os.path.isfile(android_mk) and os.path.isfile(application_mk):
        result = subprocess.call(['grep', '-q', 'import-module', android_mk])
        if result != 0:
            reason = should_skip_test(test_dir, abi, platform)
            if reason is not None:
                return Skipped(test_name, reason)

    prep_build_dir(test_dir, build_dir)
    with cd(build_dir):
        build_cmd = ['sh', 'build.sh', get_jobs_arg()] + build_flags
        if subprocess.call(build_cmd) == 0:
            return Success(test_name)
        else:
            return Failure(test_name, 'build failed')


def run_ndk_build_test(test_name, build_dir, test_dir, build_flags, abi,
                       platform):
    reason = should_skip_test(test_dir, abi, platform)
    if reason is not None:
        return Skipped(test_name, reason)

    prep_build_dir(test_dir, build_dir)
    with cd(build_dir):
        rc = ndk.build(build_flags + [get_jobs_arg()])
    expect_failure = os.path.isfile(
        os.path.join(test_dir, 'BUILD_SHOULD_FAIL'))
    if rc == 0 and expect_failure:
        return Failure(test_name, 'build should have failed')
    elif rc != 0 and not expect_failure:
        return Failure(test_name, 'build failed')
    return Success(test_name)


def run_build_test(out_dir, test_dir, build_flags, abi, platform):
    test_name = os.path.basename(test_dir)
    print('Running build test: {}'.format(test_name))

    build_dir = os.path.join(out_dir, test_name)
    if os.path.isfile(os.path.join(test_dir, 'build.sh')):
        return [run_build_sh_test(test_name, build_dir, test_dir, build_flags,
                                  abi, platform)]
    else:
        return [run_ndk_build_test(test_name, build_dir, test_dir, build_flags,
                                   abi, platform)]


def copy_test_to_device(build_dir, device_dir, abi):
    abi_dir = os.path.join(build_dir, 'libs', abi)
    if not os.path.isdir(abi_dir):
        raise RuntimeError('No libraries for {}'.format(abi))

    test_cases = []
    for test_file in os.listdir(abi_dir):
        if test_file in ('gdbserver', 'gdb.setup'):
            continue

        if not test_file.endswith('.so'):
            test_cases.append(test_file)

        # TODO(danalbert): Libs with the same name will clobber each other.
        # This was the case with the old shell based script too. I'm trying not
        # to change too much in the translation.
        lib_path = os.path.join(abi_dir, test_file)
        adb.push(lib_path, device_dir)

        # TODO(danalbert): Sync data.
        # The libc++ tests contain a DATA file that lists test names and their
        # dependencies on file system data. These files need to be copied to
        # the device.

    if len(test_cases) == 0:
        raise RuntimeError('Could not find any test executables.')

    return test_cases


def run_device_test(out_dir, test_dir, build_flags, abi, platform):
    test_name = os.path.basename(test_dir)
    build_dir = os.path.join(out_dir, test_name)
    build_result = run_ndk_build_test(test_name, build_dir, test_dir,
                                      build_flags, abi, platform)
    if not build_result.passed:
        return [build_result]

    device_dir = os.path.join('/data/local/tmp/ndk-tests', test_name)

    result, out = adb.shell('mkdir -p {}'.format(device_dir))
    if result != 0:
        raise RuntimeError('mkdir failed:\n' + '\n'.join(out))

    results = []
    try:
        test_cases = copy_test_to_device(build_dir, device_dir, abi)
        for case in test_cases:
            case_name = '.'.join([test_name, case])
            if run_is_disabled(case, test_dir):
                results.append(Skipped(case_name, 'run disabled'))
                continue

            cmd = 'cd {} && LD_LIBRARY_PATH={} ./{}'.format(
                device_dir, device_dir, case)
            result, out = adb.shell(cmd)
            if result == 0:
                results.append(Success(case_name))
            else:
                results.append(Failure(case_name, '\n'.join(out)))
        return results
    finally:
        adb.shell('rm -rf {}'.format(device_dir))


def run_tests(out_dir, test_dir, test_func):
    results = []
    for dentry in os.listdir(test_dir):
        path = os.path.join(test_dir, dentry)
        if os.path.isdir(path):
            try:
                results.extend(test_func(out_dir, path))
            except RuntimeError as ex:
                results.append(Failure(os.path.basename(dentry), ex))
    return results


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
            '--quick', action='store_true', help='Skip long running tests.')
        self.add_argument(
            '--show-all', action='store_true',
            help='Show all test results, not just failures.')


def main():
    os.chdir(os.path.dirname(os.path.realpath(__file__)))

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

    my_run_build_test = functools.partial(run_build_test,
                                          build_flags=ndk_build_flags,
                                          abi=args.abi,
                                          platform=args.platform)

    my_run_device_test = functools.partial(run_device_test,
                                           build_flags=ndk_build_flags,
                                           abi=args.abi,
                                           platform=args.platform)

    suites = ['awk', 'build', 'device', 'samples']
    if args.suite:
        suites = [args.suite]

    # Do this early so we find any device issues now rather than after we've
    # run all the build tests.
    if 'device' in suites:
        check_adb_works_or_die(args.abi)

    os.environ['ANDROID_SERIAL'] = get_test_device()

    results = {suite: [] for suite in suites}
    if 'awk' in suites:
        results['awk'] = run_tests(out_dir, 'awk', run_awk_test)
    if 'build' in suites:
        results['build'] = run_tests(out_dir, 'build', my_run_build_test)
    if 'samples' in suites:
        results['samples'] = run_tests(out_dir, '../samples',
                                       my_run_build_test)
    if 'device' in suites:
        results['device'] = run_tests(out_dir, 'device', my_run_device_test)

    num_tests = sum(len(s) for s in results.values())
    zero_stats = {'pass': 0, 'skip': 0, 'fail': 0}
    stats = {suite: dict(zero_stats) for suite in suites}
    global_stats = dict(zero_stats)
    for suite, test_results in results.items():
        for result in test_results:
            if isinstance(result, Failure):
                stats[suite]['fail'] += 1
                global_stats['fail'] += 1
            if isinstance(result, Skipped):
                stats[suite]['skip'] += 1
                global_stats['skip'] += 1
            if isinstance(result, Success):
                stats[suite]['pass'] += 1
                global_stats['pass'] += 1

    def format_stats(num_tests, stats, use_color):
        return '{pl} {p}/{t} {fl} {f}/{t} {sl} {s}/{t}'.format(
            pl=color_string('PASS', 'green') if use_color else 'PASS',
            fl=color_string('FAIL', 'red') if use_color else 'FAIL',
            sl=color_string('SKIP', 'yellow') if use_color else 'SKIP',
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
            if args.show_all or isinstance(result, Failure):
                print(result.to_string(colored=use_color))

    sys.exit(global_stats['fail'] == 0)


if __name__ == '__main__':
    main()
