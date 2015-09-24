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
from __future__ import print_function

import glob
import json
import multiprocessing
import os
import re
import shutil
import subprocess

import adb
import ndk
import util

# pylint: disable=no-self-use


def _get_jobs_arg():
    return '-j{}'.format(multiprocessing.cpu_count() * 2)


def _make_subtest_name(test, case):
    return '.'.join([test, case])


def _scan_test_suite(suite_dir, test_class, *args):
    tests = []
    for dentry in os.listdir(suite_dir):
        path = os.path.join(suite_dir, dentry)
        if os.path.isdir(path):
            tests.append(test_class.from_dir(path, *args))
    return tests


class TestRunner(object):
    def __init__(self):
        self.tests = {}

    def add_suite(self, name, path, test_class, *args):
        if name in self.tests:
            raise KeyError('suite {} already exists'.format(name))
        self.tests[name] = _scan_test_suite(path, test_class, *args)

    def _fixup_expected_failure(self, result, config, bug):
        if result.failed():
            return ExpectedFailure(result.test_name, config, bug)
        elif result.passed():
            return UnexpectedSuccess(result.test_name, config, bug)
        else:  # A skipped test case.
            return result

    def _run_test(self, test, out_dir, test_filters):
        if not test_filters.filter(test.name):
            return [Skipped(test.name, 'filtered')]

        config = test.check_unsupported()
        if config is not None:
            message = 'test unsupported for {}'.format(config)
            return [Skipped(test.name, message)]

        results = test.run(out_dir, test_filters)
        config, bug = test.check_broken()
        if config is None:
            return results

        # We need to check each individual test case for pass/fail and change
        # it to either an ExpectedFailure or an UnexpectedSuccess as necessary.
        return [self._fixup_expected_failure(r, config, bug) for r in results]

    def run(self, out_dir, test_filters):
        results = {suite: [] for suite in self.tests.keys()}
        for suite, tests in self.tests.items():
            test_results = []
            for test in tests:
                test_results.extend(self._run_test(test, out_dir,
                                                   test_filters))
            results[suite] = test_results
        return results


def _maybe_color(text, color, do_color):
    return util.color_string(text, color) if do_color else text


class TestResult(object):
    def __init__(self, test_name):
        self.test_name = test_name

    def __repr__(self):
        return self.to_string(colored=False)

    def passed(self):
        raise NotImplementedError

    def failed(self):
        raise NotImplementedError

    def to_string(self, colored=False):
        raise NotImplementedError


class Failure(TestResult):
    def __init__(self, test_name, message):
        super(Failure, self).__init__(test_name)
        self.message = message

    def passed(self):
        return False

    def failed(self):
        return True

    def to_string(self, colored=False):
        label = _maybe_color('FAIL', 'red', colored)
        return '{} {}: {}'.format(label, self.test_name, self.message)


class Success(TestResult):
    def passed(self):
        return True

    def failed(self):
        return False

    def to_string(self, colored=False):
        label = _maybe_color('PASS', 'green', colored)
        return '{} {}'.format(label, self.test_name)


class Skipped(TestResult):
    def __init__(self, test_name, reason):
        super(Skipped, self).__init__(test_name)
        self.reason = reason

    def passed(self):
        return False

    def failed(self):
        return False

    def to_string(self, colored=False):
        label = _maybe_color('SKIP', 'yellow', colored)
        return '{} {}: {}'.format(label, self.test_name, self.reason)


class ExpectedFailure(TestResult):
    def __init__(self, test_name, config, bug):
        super(ExpectedFailure, self).__init__(test_name)
        self.config = config
        self.bug = bug

    def passed(self):
        return True

    def failed(self):
        return False

    def to_string(self, colored=False):
        label = _maybe_color('KNOWN FAIL', 'yellow', colored)
        return '{} {}: known failure for {} ({})'.format(
            label, self.test_name, self.config, self.bug)


class UnexpectedSuccess(TestResult):
    def __init__(self, test_name, config, bug):
        super(UnexpectedSuccess, self).__init__(test_name)
        self.config = config
        self.bug = bug

    def passed(self):
        return False

    def failed(self):
        return True

    def to_string(self, colored=False):
        label = _maybe_color('SHOULD FAIL', 'red', colored)
        return '{} {}: unexpected success for {} ({})'.format(
            label, self.test_name, self.config, self.bug)


class Test(object):
    def __init__(self, name, test_dir):
        self.name = name
        self.test_dir = test_dir
        self.config = TestConfig.from_test_dir(self.test_dir)

    def run(self, out_dir, test_filters):
        raise NotImplementedError

    def check_broken(self):
        return self.config.match_broken(self.abi, self.platform,
                                        self.toolchain)

    def check_unsupported(self):
        return self.config.match_unsupported(self.abi, self.platform,
                                             self.toolchain)

    def check_subtest_broken(self, name):
        return self.config.match_subtest_broken(name, self.abi, self.platform,
                                                self.toolchain)

    def check_subtest_unsupported(self, name):
        return self.config.match_subtest_unsupported(
            name, self.abi, self.platform, self.toolchain)


class AwkTest(Test):
    def __init__(self, name, test_dir, script):
        super(AwkTest, self).__init__(name, test_dir)
        self.script = script

    @classmethod
    def from_dir(cls, test_dir):
        test_name = os.path.basename(test_dir)
        script_name = test_name + '.awk'
        script = os.path.join(ndk.NDK_ROOT, 'build/awk', script_name)
        if not os.path.isfile(script):
            msg = '{} missing test script: {}'.format(test_name, script)
            raise RuntimeError(msg)

        # Check that all of our test cases are valid.
        for test_case in glob.glob(os.path.join(test_dir, '*.in')):
            golden_path = re.sub(r'\.in$', '.out', test_case)
            if not os.path.isfile(golden_path):
                msg = '{} missing output: {}'.format(test_name, golden_path)
                raise RuntimeError(msg)
        return cls(test_name, test_dir, script)

    # Awk tests only run in a single configuration. Disabling them per ABI,
    # platform, or toolchain has no meaning. Stub out the checks.
    def check_broken(self):
        return None, None

    def check_unsupported(self):
        return None

    def run(self, out_dir, test_filters):
        results = []
        for test_case in glob.glob(os.path.join(self.test_dir, '*.in')):
            golden_path = re.sub(r'\.in$', '.out', test_case)
            result = self.run_case(out_dir, test_case, golden_path,
                                   test_filters)
            results.append(result)
        return results

    def run_case(self, out_dir, test_case, golden_out_path, test_filters):
        case_name = os.path.splitext(os.path.basename(test_case))[0]
        name = _make_subtest_name(self.name, case_name)

        if not test_filters.filter(name):
            return Skipped(name, 'filtered')

        out_path = os.path.join(out_dir, os.path.basename(golden_out_path))

        with open(test_case, 'r') as test_in, open(out_path, 'w') as out_file:
            awk_path = ndk.get_tool('awk')
            print('{} -f {} < {} > {}'.format(
                awk_path, self.script, test_case, out_path))
            rc = subprocess.call(['awk', '-f', self.script], stdin=test_in,
                                 stdout=out_file)
            if rc != 0:
                return Failure(name, 'awk failed')

        with open(os.devnull, 'wb') as dev_null:
            rc = subprocess.call(['cmp', out_path, golden_out_path],
                                 stdout=dev_null, stderr=dev_null)
            if rc == 0:
                return Success(name)
            else:
                p = subprocess.Popen(
                    ['diff', '-buN', out_path, golden_out_path],
                    stdout=subprocess.PIPE, stderr=dev_null)
                out, _ = p.communicate()
                if p.returncode != 0:
                    raise RuntimeError('Could not generate diff')
                message = 'output does not match expected:\n\n' + out
                return Failure(name, message)


def _prep_build_dir(src_dir, out_dir):
    if os.path.exists(out_dir):
        shutil.rmtree(out_dir)
    shutil.copytree(src_dir, out_dir)


class TestConfig(object):
    """Describes the status of a test.

    Each test directory can contain a "test_config.json" file that describes
    the configurations a test is not expected to pass for. Previously this
    information could be captured in one of two places: the Application.mk
    file, or a BROKEN_BUILD/BROKEN_RUN file.

    Application.mk was used to state that a test was only to be run for a
    specific platform version, specific toolchain, or a set of ABIs.
    Unfortunately Application.mk could only specify a single toolchain or
    platform, not a set.

    BROKEN_BUILD/BROKEN_RUN files were too general. An empty file meant the
    test should always be skipped regardless of configuration. Any change that
    would put a test in that situation should be reverted immediately. These
    also didn't make it clear if the test was actually broken (and thus should
    be fixed) or just not applicable.

    A test_config.json file is more descriptive. It can mark a test as
    "broken" or "unsupported", and can do so with a list of "abis",
    "platforms", or "toolchains". It is assumed that most tests are valid for
    any configuration. Such tests require no test_config.json file.

    Types of tests:
        Fully passing tests: No metadata file required.

        broken: known failing tests that should be fixed:
            Tests that might fail for a specific architecture or platform
            version.  Any test with a non-empty broken section requires a "bug"
            entry with a link to either an internal bug (http://b/BUG_NUMBER)
            or a public bug (http://b.android.com/BUG_NUMBER).

            These tests will still be built and run. If the test succeeds, it
            will be reported as an error.

            Example metadata file:
            {
                "broken": {
                    "bug": "http://b/123456",
                    "toolchains": ["clang3.5", "4.9"]
                }
            }

        unsupported: tests that are not meaningful for the given configuration:
            Tests that do not make sense to run for a given configuration.
            Testing x86 assembler on MIPS, for example.

            These tests will not be built or run.

            Example metadata file:
            {
                "unsupported": {
                    "platforms": ["android-10", "android-14"]
                }
            }

        requires: tests that are only meaningful for the given configuration:
            This is the inverse of unsupported. Any configurations provided
            will be inverted and then used that same as before. Note that
            unsupported and requires may not be used together.

            Example metadata file:
            {
                "requires": {
                    "abis": ["mips"]
                }
            }

    Configuration specifiers:
        These are json keys that are used to specify the conditions for which a
        "broken" or "unsupported" clause applies. Note that these apply for an
        any match, not an all. There is no way to specify that something is
        broken with, for example, only armeabi on android-10.

        abis: applies for the listed ABIs
        platforms: applies for the listed platforms
        toolchains: applies for the listed toolchains

    Subtests:
        Device tests often contain more than one test case. Each executable
        defined in an Android.mk is considered a separate test case. Any one of
        these might be broken or unsupported, and we need to be able to mark
        them individually. To do so, each test may be listed in the "subtests"
        list.

        Example metadata file:
        {
            "subtests": {
                "test-foo": {
                    "broken": {
                        "abis": ["x86"],
                        "bug": "http://b/123456"
                    }
                }
            }
        }
    """

    ALL_ABIS = {
        'armeabi',
        'armeabi-v7a',
        'armeabi-v7a-hard',
        'arm64-v8a',
        'mips',
        'mips64',
        'x86',
        'x86_64',
    }

    ALL_PLATFORMS = set('android-' + str(v) for v in range(9, 21))

    ALL_TOOLCHAINS = {'4.9', 'clang3.6'}

    def __init__(self, file_name, json_data):
        self.file_name = file_name
        self.data = json_data
        self._validate_data()
        self._requires_to_unsupported()

    def _for_each_section(self, func):
        func(self.data, False)
        if 'subtests' in self.data:
            for subsection in self.data['subtests'].items():
                func(subsection, True)

    def _validate_data(self):
        def verifier(section, is_subtest):
            return self._validate_test_data(section, is_subtest)
        self._for_each_section(verifier)

    def _validate_test_data(self, section, is_subtest):
        if 'broken' in section and 'bug' not in section['broken']:
            message = 'broken test has no bug number: ' + self.file_name
            raise ValueError(message)

        if 'requires' in section and 'unsupported' in section:
            message = 'requires conflicts with unsupported: ' + self.file_name
            raise ValueError(message)

        if is_subtest and 'subtests' in section:
            message = 'found invalid "subtests" section: ' + self.file_name
            raise ValueError(message)

    def _requires_to_unsupported(self):
        def converter(section, _):
            if 'requires' in section:
                requires = section.pop('requires')
                unsupported = {}

                if 'abis' in requires:
                    unsupported['abis'] = list(
                        self.ALL_ABIS - set(requires['abis']))
                if 'platforms' in requires:
                    unsupported['platforms'] = list(
                        self.ALL_PLATFORMS - set(requires['platforms']))
                if 'toolchains' in requires:
                    unsupported['toolchains'] = list(
                        self.ALL_TOOLCHAINS - set(requires['toolchains']))

                section['unsupported'] = unsupported
        self._for_each_section(converter)

    @classmethod
    def from_test_dir(cls, test_dir):
        path = os.path.join(test_dir, 'test_config.json')
        if os.path.exists(path):
            with open(path) as metadata_file:
                metadata = json.load(metadata_file)
        else:
            metadata = {}
        return cls(path, metadata)

    def _match(self, group, abi, platform, toolchain):
        if abi in group.get('abis', []):
            return abi
        if platform in group.get('platforms', []):
            return platform
        if toolchain in group.get('toolchains', []):
            return toolchain

    def match_broken(self, abi, platform, toolchain):
        """Checks if a test is broken under the given configuration.

        Returns: tuple of (matched_config, bug_number), both as strings, or
                 (None, None) if there was no match.
        """
        if 'broken' not in self.data:
            return None, None

        group = self.data['broken']
        broken_config = self._match(group, abi, platform, toolchain)
        if broken_config is None:
            return None, None

        return broken_config, group['bug']

    def match_unsupported(self, abi, platform, toolchain):
        """Checks if a test is unsupported under the given configuration.

        Returns: string representation of the unsupported configuration or
                 None if there was no match.
        """
        if 'unsupported' not in self.data:
            return None

        group = self.data['unsupported']
        return self._match(group, abi, platform, toolchain)

    def _get_subtest_group(self, name, group):
        if 'subtests' not in self.data:
            return None

        if name not in self.data['subtests']:
            return None

        return self.data['subtests'][name].get(group, None)

    def match_subtest_broken(self, name, abi, platform, toolchain):
        group = self._get_subtest_group(name, 'broken')
        if group is None:
            return None, None

        broken_config = self._match(group, abi, platform, toolchain)
        if broken_config is None:
            return None, None

        return broken_config, group['bug']

    def match_subtest_unsupported(self, name, abi, platform, toolchain):
        group = self._get_subtest_group(name, 'unsupported')
        if group is None:
            return None
        return self._match(group, abi, platform, toolchain)


def _run_build_sh_test(test_name, build_dir, test_dir, build_flags, abi,
                       platform, toolchain):
    _prep_build_dir(test_dir, build_dir)
    with util.cd(build_dir):
        build_cmd = ['sh', 'build.sh', _get_jobs_arg()] + build_flags
        test_env = dict(os.environ)
        if abi is not None:
            test_env['APP_ABI'] = abi
        if platform is not None:
            test_env['APP_PLATFORM'] = platform
        assert toolchain is not None
        test_env['NDK_TOOLCHAIN_VERSION'] = toolchain
        if subprocess.call(build_cmd, env=test_env) == 0:
            return Success(test_name)
        else:
            return Failure(test_name, 'build failed')


def _run_ndk_build_test(test_name, build_dir, test_dir, build_flags, abi,
                        platform, toolchain):
    _prep_build_dir(test_dir, build_dir)
    with util.cd(build_dir):
        args = [
            'APP_ABI=' + abi,
            'NDK_TOOLCHAIN_VERSION=' + toolchain,
            _get_jobs_arg(),
        ]
        if platform is not None:
            args.append('APP_PLATFORM=' + platform)
        if ndk.build(build_flags + args) == 0:
            return Success(test_name)
        else:
            return Failure(test_name, 'build failed')


class ShellBuildTest(Test):
    def __init__(self, name, test_dir, abi, platform, toolchain, build_flags):
        super(ShellBuildTest, self).__init__(name, test_dir)
        self.abi = abi
        self.platform = platform
        self.toolchain = toolchain
        self.build_flags = build_flags

    def run(self, out_dir, _):
        build_dir = os.path.join(out_dir, self.name)
        print('Running build test: {}'.format(self.name))
        return [_run_build_sh_test(self.name, build_dir, self.test_dir,
                                   self.build_flags, self.abi, self.platform,
                                   self.toolchain)]


class NdkBuildTest(Test):
    def __init__(self, name, test_dir, abi, platform, toolchain, build_flags):
        super(NdkBuildTest, self).__init__(name, test_dir)
        self.abi = abi
        self.platform = platform
        self.toolchain = toolchain
        self.build_flags = build_flags

    def run(self, out_dir, _):
        build_dir = os.path.join(out_dir, self.name)
        print('Running build test: {}'.format(self.name))
        return [_run_ndk_build_test(self.name, build_dir, self.test_dir,
                                    self.build_flags, self.abi,
                                    self.platform, self.toolchain)]


class BuildTest(object):
    @classmethod
    def from_dir(cls, test_dir, abi, platform, toolchain, build_flags):
        test_name = os.path.basename(test_dir)

        if os.path.isfile(os.path.join(test_dir, 'build.sh')):
            return ShellBuildTest(test_name, test_dir, abi, platform,
                                  toolchain, build_flags)
        else:
            return NdkBuildTest(test_name, test_dir, abi, platform,
                                toolchain, build_flags)


def _copy_test_to_device(build_dir, device_dir, abi, test_filters, test_name):
    abi_dir = os.path.join(build_dir, 'libs', abi)
    if not os.path.isdir(abi_dir):
        raise RuntimeError('No libraries for {}'.format(abi))

    test_cases = []
    for test_file in os.listdir(abi_dir):
        if test_file in ('gdbserver', 'gdb.setup'):
            continue

        if not test_file.endswith('.so'):
            case_name = _make_subtest_name(test_name, test_file)
            if not test_filters.filter(case_name):
                continue
            test_cases.append(test_file)

        # TODO(danalbert): Libs with the same name will clobber each other.
        # This was the case with the old shell based script too. I'm trying not
        # to change too much in the translation.
        lib_path = os.path.join(abi_dir, test_file)
        print('Pushing {} to {}'.format(lib_path, device_dir))
        adb.push(lib_path, device_dir)

        # TODO(danalbert): Sync data.
        # The libc++ tests contain a DATA file that lists test names and their
        # dependencies on file system data. These files need to be copied to
        # the device.

    if len(test_cases) == 0:
        raise RuntimeError('Could not find any test executables.')

    return test_cases


class DeviceTest(Test):
    def __init__(self, name, test_dir, abi, platform, toolchain, build_flags):
        super(DeviceTest, self).__init__(name, test_dir)
        self.abi = abi
        self.platform = platform
        self.toolchain = toolchain
        self.build_flags = build_flags

    @classmethod
    def from_dir(cls, test_dir, abi, platform, toolchain, build_flags):
        test_name = os.path.basename(test_dir)
        return cls(test_name, test_dir, abi, platform, toolchain, build_flags)

    def run(self, out_dir, test_filters):
        print('Running device test: {}'.format(self.name))
        build_dir = os.path.join(out_dir, self.name)
        build_result = _run_ndk_build_test(self.name, build_dir, self.test_dir,
                                           self.build_flags, self.abi,
                                           self.platform, self.toolchain)
        if not build_result.passed():
            return [build_result]

        device_dir = os.path.join('/data/local/tmp/ndk-tests', self.name)

        result, out = adb.shell('mkdir -p {}'.format(device_dir))
        if result != 0:
            raise RuntimeError('mkdir failed:\n' + '\n'.join(out))

        results = []
        try:
            test_cases = _copy_test_to_device(
                build_dir, device_dir, self.abi, test_filters, self.name)
            for case in test_cases:
                case_name = _make_subtest_name(self.name, case)
                if not test_filters.filter(case_name):
                    results.append(Skipped(case_name, 'filtered'))
                    continue

                config = self.check_subtest_unsupported(case)
                if config is not None:
                    message = 'test unsupported for {}'.format(config)
                    results.append(Skipped(case_name, message))
                    continue

                cmd = 'cd {} && LD_LIBRARY_PATH={} ./{}'.format(
                    device_dir, device_dir, case)
                print('Executing test: {}'.format(cmd))
                result, out = adb.shell(cmd)

                config, bug = self.check_subtest_broken(case)
                if config is None:
                    if result == 0:
                        results.append(Success(case_name))
                    else:
                        results.append(Failure(case_name, '\n'.join(out)))
                else:
                    if result == 0:
                        results.append(UnexpectedSuccess(case_name, config,
                                                         bug))
                    else:
                        results.append(ExpectedFailure(case_name, config, bug))
            return results
        finally:
            adb.shell('rm -rf {}'.format(device_dir))
