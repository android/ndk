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

import difflib
import filecmp
import glob
import imp
import multiprocessing
import os
import posixpath
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
        if isinstance(result, Failure):
            return ExpectedFailure(result.test_name, config, bug)
        elif isinstance(result, Success):
            return UnexpectedSuccess(result.test_name, config, bug)
        else:  # Skipped, UnexpectedSuccess, or ExpectedFailure.
            return result

    def _run_test(self, test, out_dir, test_filters):
        if not test_filters.filter(test.name):
            return []

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
        self.config = self.get_test_config()

    def get_test_config(self):
        return TestConfig.from_test_dir(self.test_dir)

    def run(self, out_dir, test_filters):
        raise NotImplementedError

    def check_broken(self):
        return self.config.match_broken(self.abi, self.platform,
                                        self.toolchain)

    def check_unsupported(self):
        return self.config.match_unsupported(self.abi, self.platform,
                                             self.toolchain)

    def check_subtest_broken(self, name):
        return self.config.match_broken(self.abi, self.platform,
                                        self.toolchain, subtest=name)

    def check_subtest_unsupported(self, name):
        return self.config.match_unsupported(self.abi, self.platform,
                                             self.toolchain, subtest=name)


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
            if result is not None:
                results.append(result)
        return results

    def run_case(self, out_dir, test_case, golden_out_path, test_filters):
        case_name = os.path.splitext(os.path.basename(test_case))[0]
        name = _make_subtest_name(self.name, case_name)

        if not test_filters.filter(name):
            return None

        out_path = os.path.join(out_dir, os.path.basename(golden_out_path))

        with open(test_case, 'r') as test_in, open(out_path, 'w') as out_file:
            awk_path = ndk.get_tool('awk')
            print('{} -f {} < {} > {}'.format(
                awk_path, self.script, test_case, out_path))
            rc = subprocess.call([awk_path, '-f', self.script], stdin=test_in,
                                 stdout=out_file)
            if rc != 0:
                return Failure(name, 'awk failed')

        if filecmp.cmp(out_path, golden_out_path):
            return Success(name)
        else:
            with open(out_path) as out_file:
                out_lines = out_file.readlines()
            with open(golden_out_path) as golden_out_file:
                golden_lines = golden_out_file.readlines()
            diff = ''.join(difflib.unified_diff(
                golden_lines, out_lines, fromfile='expected', tofile='actual'))
            message = 'output does not match expected:\n\n' + diff
            return Failure(name, message)


def _prep_build_dir(src_dir, out_dir):
    if os.path.exists(out_dir):
        shutil.rmtree(out_dir)
    shutil.copytree(src_dir, out_dir)


class TestConfig(object):
    """Describes the status of a test.

    Each test directory can contain a "test_config.py" file that describes
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

    A test_config.py file is more flexible. It is a Python module that defines
    at least one function by the same name as one in TestConfig.NullTestConfig.
    If a function is not defined the null implementation (not broken,
    supported), will be used.
    """

    class NullTestConfig(object):
        def __init__(self):
            pass

        # pylint: disable=unused-argument
        @staticmethod
        def match_broken(abi, platform, toolchain, subtest=None):
            """Tests if a given configuration is known broken.

            A broken test is a known failing test that should be fixed.

            Any test with a non-empty broken section requires a "bug" entry
            with a link to either an internal bug (http://b/BUG_NUMBER) or a
            public bug (http://b.android.com/BUG_NUMBER).

            These tests will still be built and run. If the test succeeds, it
            will be reported as an error.

            Returns: A tuple of (broken_configuration, bug) or (None, None).
            """
            return None, None

        @staticmethod
        def match_unsupported(abi, platform, toolchain, subtest=None):
            """Tests if a given configuration is unsupported.

            An unsupported test is a test that do not make sense to run for a
            given configuration. Testing x86 assembler on MIPS, for example.

            These tests will not be built or run.

            Returns: The string unsupported_configuration or None.
            """
            return None
        # pylint: enable=unused-argument

    def __init__(self, file_path):

        # Note that this namespace isn't actually meaningful from our side;
        # it's only what the loaded module's __name__ gets set to.
        dirname = os.path.dirname(file_path)
        namespace = '.'.join([dirname, 'test_config'])

        try:
            self.module = imp.load_source(namespace, file_path)
        except IOError:
            self.module = None

        try:
            self.match_broken = self.module.match_broken
        except AttributeError:
            self.match_broken = self.NullTestConfig.match_broken

        try:
            self.match_unsupported = self.module.match_unsupported
        except AttributeError:
            self.match_unsupported = self.NullTestConfig.match_unsupported

    @classmethod
    def from_test_dir(cls, test_dir):
        path = os.path.join(test_dir, 'test_config.py')
        return cls(path)


class DeviceTestConfig(TestConfig):
    """Specialization of test_config.py that includes device API level.

    We need to mark some tests as broken or unsupported based on what device
    they are running on, as opposed to just what they were built for.
    """
    class NullTestConfig(object):
        def __init__(self):
            pass

        # pylint: disable=unused-argument
        @staticmethod
        def match_broken(abi, platform, device_platform, toolchain,
                         subtest=None):
            return None, None

        @staticmethod
        def match_unsupported(abi, platform, device_platform, toolchain,
                              subtest=None):
            return None
        # pylint: enable=unused-argument


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
        rc, out = util.call_output(build_cmd, env=test_env)
        if rc == 0:
            return Success(test_name)
        else:
            return Failure(test_name, out)


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
        rc, out = ndk.build(build_flags + args)
        if rc == 0:
            return Success(test_name)
        else:
            return Failure(test_name, out)


class PythonBuildTest(Test):
    """A test that is implemented by test.py.

    A test.py test has a test.py file in its root directory. This module
    contains a run_test function which returns a tuple of `(boolean_success,
    string_failure_message)` and takes the following kwargs (all of which
    default to None):

    abi: ABI to test as a string.
    platform: Platform to build against as a string.
    toolchain: Toolchain to use as a string.
    build_flags: Additional build flags that should be passed to ndk-build if
                 invoked as a list of strings.
    """
    def __init__(self, name, test_dir, abi, platform, toolchain, build_flags):
        super(PythonBuildTest, self).__init__(name, test_dir)
        self.abi = abi
        self.platform = platform
        self.toolchain = toolchain
        self.build_flags = build_flags

    def run(self, out_dir, _):
        build_dir = os.path.join(out_dir, self.name)
        print('Running build test: {}'.format(self.name))
        _prep_build_dir(self.test_dir, build_dir)
        with util.cd(build_dir):
            module = imp.load_source('test', 'test.py')
            success, failure_message = module.run_test(
                abi=self.abi, platform=self.platform, toolchain=self.toolchain,
                build_flags=self.build_flags)
            if success:
                return [Success(self.name)]
            else:
                return [Failure(self.name, failure_message)]


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
        if os.name == 'nt':
            reason = 'build.sh tests are not supported on Windows'
            return [Skipped(self.name, reason)]
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

        if os.path.isfile(os.path.join(test_dir, 'test.py')):
            return PythonBuildTest(test_name, test_dir, abi, platform,
                                   toolchain, build_flags)
        elif os.path.isfile(os.path.join(test_dir, 'build.sh')):
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

        file_is_lib = False
        if not test_file.endswith('.so'):
            file_is_lib = True
            case_name = _make_subtest_name(test_name, test_file)
            if not test_filters.filter(case_name):
                continue
            test_cases.append(test_file)

        # TODO(danalbert): Libs with the same name will clobber each other.
        # This was the case with the old shell based script too. I'm trying not
        # to change too much in the translation.
        lib_path = os.path.join(abi_dir, test_file)
        print('\tPushing {} to {}...'.format(lib_path, device_dir))
        adb.push(lib_path, device_dir)

        # Binaries pushed from Windows may not have execute permissions.
        if not file_is_lib:
            file_path = posixpath.join(device_dir, test_file)
            adb.shell('chmod +x ' + file_path)

        # TODO(danalbert): Sync data.
        # The libc++ tests contain a DATA file that lists test names and their
        # dependencies on file system data. These files need to be copied to
        # the device.

    if len(test_cases) == 0:
        raise RuntimeError('Could not find any test executables.')

    return test_cases


class DeviceTest(Test):
    def __init__(self, name, test_dir, abi, platform, device_platform,
                 toolchain, build_flags):
        super(DeviceTest, self).__init__(name, test_dir)
        self.abi = abi
        self.platform = platform
        self.device_platform = device_platform
        self.toolchain = toolchain
        self.build_flags = build_flags

    @classmethod
    def from_dir(cls, test_dir, abi, platform, device_platform, toolchain,
                 build_flags):
        test_name = os.path.basename(test_dir)
        return cls(test_name, test_dir, abi, platform, device_platform,
                   toolchain, build_flags)

    def get_test_config(self):
        return DeviceTestConfig.from_test_dir(self.test_dir)

    def check_broken(self):
        return self.config.match_broken(self.abi, self.platform,
                                        self.device_platform,
                                        self.toolchain)

    def check_unsupported(self):
        return self.config.match_unsupported(self.abi, self.platform,
                                             self.device_platform,
                                             self.toolchain)

    def check_subtest_broken(self, name):
        return self.config.match_broken(self.abi, self.platform,
                                        self.device_platform,
                                        self.toolchain, subtest=name)

    def check_subtest_unsupported(self, name):
        return self.config.match_unsupported(self.abi, self.platform,
                                             self.device_platform,
                                             self.toolchain, subtest=name)

    def run(self, out_dir, test_filters):
        print('Running device test: {}'.format(self.name))
        build_dir = os.path.join(out_dir, self.name)
        build_result = _run_ndk_build_test(self.name, build_dir, self.test_dir,
                                           self.build_flags, self.abi,
                                           self.platform, self.toolchain)
        if not build_result.passed():
            return [build_result]

        device_dir = posixpath.join('/data/local/tmp/ndk-tests', self.name)

        # We have to use `ls foo || mkdir foo` because Gingerbread was lacking
        # `mkdir -p`, the -d check for directory existence, stat, dirname, and
        # every other thing I could think of to implement this aside from ls.
        result, out = adb.shell('ls {0} || mkdir {0}'.format(device_dir))
        if result != 0:
            raise RuntimeError('mkdir failed:\n' + '\n'.join(out))

        results = []
        try:
            test_cases = _copy_test_to_device(
                build_dir, device_dir, self.abi, test_filters, self.name)
            for case in test_cases:
                case_name = _make_subtest_name(self.name, case)
                if not test_filters.filter(case_name):
                    continue

                config = self.check_subtest_unsupported(case)
                if config is not None:
                    message = 'test unsupported for {}'.format(config)
                    results.append(Skipped(case_name, message))
                    continue

                cmd = 'cd {} && LD_LIBRARY_PATH={} ./{}'.format(
                    device_dir, device_dir, case)
                print('\tExecuting {}...'.format(case_name))
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
            adb.shell('rm -r {}'.format(device_dir))
