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
"""Verifies that the build is sane.

Cleans old build artifacts, configures the required environment, determines
build goals, and invokes the build scripts.
"""
from __future__ import print_function

import argparse
import collections
import datetime
import inspect
import os
import shutil
import site
import subprocess
import sys
import tempfile
import textwrap

site.addsitedir(os.path.join(os.path.dirname(__file__), 'build/lib'))

import build_support  # pylint: disable=import-error


ALL_MODULES = {
    'binutils',
    'build',
    'clang',
    'cpufeatures',
    'gabi++',
    'gcc',
    'gcclibs',
    'gdbserver',
    'gnustl',
    'gtest',
    'host-tools',
    'libandroid_support',
    'libc++',
    'libc++abi',
    'native_app_glue',
    'ndk_helper',
    'platforms',
    'python-packages',
    'stlport',
    'system-stl',
}


class ArgParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgParser, self).__init__(
            description=inspect.getdoc(sys.modules[__name__]))

        self.add_argument(
            '--arch',
            choices=('arm', 'arm64', 'mips', 'mips64', 'x86', 'x86_64'),
            help='Build for the given architecture. Build all by default.')

        package_group = self.add_mutually_exclusive_group()
        package_group.add_argument(
            '--package', action='store_true', dest='package', default=True,
            help='Package the NDK when done building (default).')
        package_group.add_argument(
            '--no-package', action='store_false', dest='package',
            help='Do not package the NDK when done building.')

        test_group = self.add_mutually_exclusive_group()
        test_group.add_argument(
            '--test', action='store_true', dest='test', default=True,
            help=textwrap.dedent("""\
            Run host tests when finished. --package is required. Not supported
            when targeting Windows.
            """))
        test_group.add_argument(
            '--no-test', action='store_false', dest='test',
            help='Do not run host tests when finished.')

        self.add_argument(
            '--release',
            help='Release name. Package will be named android-ndk-RELEASE.')

        self.add_argument(
            '--system', choices=('darwin', 'linux', 'windows', 'windows64'),
            default=build_support.get_default_host(),
            help='Build for the given OS.')

        module_group = self.add_mutually_exclusive_group()

        module_group.add_argument(
            '--module', choices=sorted(ALL_MODULES),
            help='NDK modules to build.')

        module_group.add_argument(
            '--host-only', action='store_true',
            help='Skip building target components.')


def _invoke_build(script, args):
    if args is None:
        args = []
    subprocess.check_call([build_support.android_path(script)] + args)


def invoke_build(script, args=None):
    script_path = os.path.join('build/tools', script)
    _invoke_build(build_support.ndk_path(script_path), args)


def invoke_external_build(script, args=None):
    _invoke_build(build_support.android_path(script), args)


def package_ndk(out_dir, dist_dir, args):
    package_args = common_build_args(out_dir, dist_dir, args)
    package_args.append(dist_dir)

    if args.release is not None:
        package_args.append('--release={}'.format(args.release))

    if args.arch is not None:
        package_args.append('--arch={}'.format(args.arch))

    invoke_build('package.py', package_args)


def test_ndk(out_dir, args):
    release = args.release
    if args.release is None:
        release = datetime.date.today().strftime('%Y%m%d')

    # The packaging step extracts all the modules to a known directory for
    # packaging. This directory is not cleaned up after packaging, so we can
    # reuse that for testing.
    test_dir = os.path.join(out_dir, 'android-ndk-{}'.format(release))

    test_env = dict(os.environ)
    test_env['NDK'] = test_dir

    abis = build_support.ALL_ABIS
    if args.arch is not None:
        abis = build_support.arch_to_abis(args.arch)

    results = {}
    for abi in abis:
        cmd = [
            'python', build_support.ndk_path('tests/run-all.py'),
            '--abi', abi, '--suite', 'build'
        ]
        print('Running tests: {}'.format(' '.join(cmd)))
        result = subprocess.call(cmd, env=test_env)
        results[abi] = result == 0

    print('Results:')
    for abi, result in results.iteritems():
        print('{}: {}'.format(abi, 'PASS' if result else 'FAIL'))
    return all(results.values())


def common_build_args(out_dir, dist_dir, args):
    build_args = ['--out-dir={}'.format(out_dir)]
    build_args = ['--dist-dir={}'.format(dist_dir)]
    build_args.append('--host={}'.format(args.system))
    return build_args


def fixup_toolchain_triple(toolchain):
    """Maps toolchain names to their proper triple.

    The x86 toolchains are named stupidly and aren't a proper triple.
    """
    return {
        'x86': 'i686-linux-android',
        'x86_64': 'x86_64-linux-android',
    }.get(toolchain, toolchain)


def get_binutils_files(triple, has_gold, is_windows):
    files = [
        'ld.bfd',
        'nm',
        'as',
        'objcopy',
        'strip',
        'objdump',
        'ld',
        'ar',
        'ranlib',
    ]

    if has_gold:
        files.append('ld.gold')

    if is_windows:
        files = [f + '.exe' for f in files]

    # binutils programs get installed to two locations:
    # 1: $INSTALL_DIR/bin/$TRIPLE-$PROGRAM
    # 2: $INSTALL_DIR/$TRIPLE/bin/$PROGRAM
    #
    # We need to copy both.

    prefixed_files = []
    for file_name in files:
        prefixed_name = '-'.join([triple, file_name])
        prefixed_files.append(os.path.join('bin', prefixed_name))

    dir_prefixed_files = []
    for file_name in files:
        dir_prefixed_files.append(os.path.join(triple, 'bin', file_name))

    ldscripts_dir = os.path.join(triple, 'lib/ldscripts')
    return prefixed_files + dir_prefixed_files + [ldscripts_dir]


def install_file(file_name, src_dir, dst_dir):
    src_file = os.path.join(src_dir, file_name)
    dst_file = os.path.join(dst_dir, file_name)

    print('Copying {} to {}...'.format(src_file, dst_file))
    if os.path.isdir(src_file):
        _install_dir(src_file, dst_file)
    elif os.path.islink(src_file):
        _install_symlink(src_file, dst_file)
    else:
        _install_file(src_file, dst_file)


def _install_dir(src_dir, dst_dir):
    parent_dir = os.path.normpath(os.path.join(dst_dir, '..'))
    if not os.path.exists(parent_dir):
        os.makedirs(parent_dir)
    shutil.copytree(src_dir, dst_dir, symlinks=True)


def _install_symlink(src_file, dst_file):
    dirname = os.path.dirname(dst_file)
    if not os.path.exists(dirname):
        os.makedirs(dirname)
    link_target = os.readlink(src_file)
    os.symlink(link_target, dst_file)


def _install_file(src_file, dst_file):
    dirname = os.path.dirname(dst_file)
    if not os.path.exists(dirname):
        os.makedirs(dirname)
    # copy2 is just copy followed by copystat (preserves file metadata).
    shutil.copy2(src_file, dst_file)


def pack_binutils(arch, host_tag, out_dir, binutils_path):
    archive_name = '-'.join(['binutils', arch, host_tag])
    build_support.make_package(archive_name, binutils_path, out_dir)


def get_prebuilt_gcc(host, arch):
    tag = build_support.host_to_tag(host)
    system_subdir = 'prebuilts/ndk/current/toolchains/{}'.format(tag)
    system_path = build_support.android_path(system_subdir)
    toolchain = build_support.arch_to_toolchain(arch)
    toolchain_dir = toolchain + '-4.9'
    return os.path.join(system_path, toolchain_dir)


def build_binutils(out_dir, dist_dir, args):
    print('Extracting binutils package from GCC...')

    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    host_tag = build_support.host_to_tag(args.system)

    for arch in arches:
        toolchain = build_support.arch_to_toolchain(arch)
        toolchain_path = get_prebuilt_gcc(args.system, arch)

        triple = fixup_toolchain_triple(toolchain)

        install_dir = os.path.join(out_dir, 'binutils', triple)
        if os.path.exists(install_dir):
            shutil.rmtree(install_dir)
        os.makedirs(install_dir)

        has_gold = True
        if host_tag == 'windows':
            # Note: 64-bit Windows is fine.
            has_gold = False
        if arch in ('mips', 'mips64'):
            has_gold = False

        is_windows = host_tag.startswith('windows')
        for file_name in get_binutils_files(triple, has_gold, is_windows):
            install_file(file_name, toolchain_path, install_dir)

        license_path = build_support.android_path(
            'toolchain/binutils/binutils-2.25/COPYING')
        shutil.copy2(license_path, os.path.join(install_dir, 'NOTICE'))

        pack_binutils(arch, host_tag, dist_dir, install_dir)


def build_clang(out_dir, dist_dir, args):
    print('Building Clang...')
    invoke_build('build-llvm.py', common_build_args(out_dir, dist_dir, args))


def build_gcc(out_dir, dist_dir, args):
    print('Building GCC...')
    build_args = common_build_args(out_dir, dist_dir, args)
    if args.arch is not None:
        build_args.append('--arch={}'.format(args.arch))
    invoke_build('build-gcc.py', build_args)


def build_gcc_libs(out_dir, dist_dir, args):
    print('Packaging GCC libs...')

    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    for arch in arches:
        toolchain = build_support.arch_to_toolchain(arch)
        triple = fixup_toolchain_triple(toolchain)
        libgcc_subdir = 'lib/gcc/{}/4.9'.format(triple)
        is64 = arch.endswith('64')
        libatomic_subdir = '{}/lib{}'.format(triple, '64' if is64 else '')

        lib_names = [
            (libatomic_subdir, 'libatomic.a'),
            (libgcc_subdir, 'libgcc.a'),
        ]

        lib_dirs = ['']
        if arch == 'arm':
            lib_dirs += [
                'armv7-a',
                'armv7-a/hard',
                'armv7-a/thumb',
                'armv7-a/thumb/hard',
                'thumb',
            ]

        libs = []
        for lib_dir in lib_dirs:
            for subdir, lib in lib_names:
                libs.append((subdir, os.path.join(lib_dir, lib)))

        install_dir = os.path.join(out_dir, 'gcclibs', triple)
        if os.path.exists(install_dir):
            shutil.rmtree(install_dir)
        os.makedirs(install_dir)

        # These are target libraries, so the OS we use here is not
        # important. We explicitly use Linux because for whatever reason
        # the Windows aarch64 toolchain doesn't include libatomic.
        gcc_path = get_prebuilt_gcc('linux', arch)
        for gcc_subdir, lib in libs:
            src = os.path.join(gcc_path, gcc_subdir, lib)
            dst = os.path.join(install_dir, lib)
            dst_dir = os.path.dirname(dst)
            if not os.path.exists(dst_dir):
                os.makedirs(dst_dir)
            shutil.copy2(src, dst)

            shutil.copy2(
                os.path.join(gcc_path, 'NOTICE'),
                os.path.join(install_dir, 'NOTICE'))

        archive_name = os.path.join('gcclibs-' + arch)
        build_support.make_package(archive_name, install_dir, dist_dir)


def build_host_tools(out_dir, dist_dir, args):
    build_args = common_build_args(out_dir, dist_dir, args)

    print('Building ndk-stack...')
    invoke_external_build(
        'ndk/sources/host-tools/ndk-stack/build.py', build_args)

    print('Building ndk-depends...')
    invoke_external_build(
        'ndk/sources/host-tools/ndk-depends/build.py', build_args)

    print('Building awk...')
    invoke_external_build(
        'ndk/sources/host-tools/nawk-20071023/build.py', build_args)

    print('Building make...')
    invoke_external_build(
        'ndk/sources/host-tools/make-3.81/build.py', build_args)

    if args.system in ('windows', 'windows64'):
        print('Building toolbox...')
        invoke_external_build(
            'ndk/sources/host-tools/toolbox/build.py', build_args)

    print('Building Python...')
    invoke_external_build('toolchain/python/build.py', build_args)

    print('Building GDB...')
    invoke_external_build('toolchain/gdb/build.py', build_args)

    print('Building YASM...')
    invoke_external_build('toolchain/yasm/build.py', build_args)

    package_host_tools(out_dir, dist_dir, args.system)


def merge_license_files(output_path, files):
    licenses = []
    for license_path in files:
        with open(license_path) as license_file:
            licenses.append(license_file.read())

    with open(output_path, 'w') as output_file:
        output_file.write('\n'.join(licenses))


def package_host_tools(out_dir, dist_dir, host):
    packages = [
        'gdb-multiarch-7.10',
        'ndk-awk',
        'ndk-depends',
        'ndk-make',
        'ndk-python',
        'ndk-stack',
        'ndk-yasm',
    ]

    files = [
        'ndk-gdb',
        'ndk-gdb.cmd',
        'ndk-gdb.py',
    ]

    if host in ('windows', 'windows64'):
        packages.append('toolbox')

    host_tag = build_support.host_to_tag(host)

    package_names = [p + '-' + host_tag + '.tar.bz2' for p in packages]
    for package_name in package_names:
        package_path = os.path.join(out_dir, package_name)
        subprocess.check_call(['tar', 'xf', package_path, '-C', out_dir])

    for f in files:
        shutil.copy2(f, os.path.join(out_dir, 'host-tools/bin'))

    merge_license_files(os.path.join(out_dir, 'host-tools/NOTICE'), [
        build_support.android_path('toolchain/gdb/gdb-7.10/COPYING'),
        build_support.ndk_path('sources/host-tools/nawk-20071023/NOTICE'),
        build_support.ndk_path('sources/host-tools/ndk-depends/NOTICE'),
        build_support.ndk_path('sources/host-tools/make-3.81/COPYING'),
        build_support.android_path(
            'toolchain/python/Python-2.7.5/LICENSE'),
        build_support.ndk_path('sources/host-tools/ndk-stack/NOTICE'),
        build_support.ndk_path('sources/host-tools/toolbox/NOTICE'),
        build_support.android_path('toolchain/yasm/COPYING'),
        build_support.android_path('toolchain/yasm/BSD.txt'),
        build_support.android_path('toolchain/yasm/Artistic.txt'),
        build_support.android_path('toolchain/yasm/GNU_GPL-2.0'),
        build_support.android_path('toolchain/yasm/GNU_LGPL-2.0'),
    ])

    package_name = 'host-tools-' + host_tag
    path = os.path.join(out_dir, 'host-tools')
    build_support.make_package(package_name, path, dist_dir)


def build_gdbserver(out_dir, dist_dir, args):
    print('Building gdbserver...')
    build_args = common_build_args(out_dir, dist_dir, args)
    if args.arch is not None:
        build_args.append('--arch={}'.format(args.arch))
    invoke_build('build-gdbserver.py', build_args)


def _build_stl(out_dir, dist_dir, args, stl):
    build_args = common_build_args(out_dir, dist_dir, args)
    if args.arch is not None:
        build_args.append('--arch={}'.format(args.arch))
    script = 'ndk/sources/cxx-stl/{}/build.py'.format(stl)
    invoke_external_build(script, build_args)


def build_gnustl(out_dir, dist_dir, args):
    print('Building gnustl...')
    _build_stl(out_dir, dist_dir, args, 'gnu-libstdc++')


def build_libcxx(out_dir, dist_dir, args):
    print('Building libc++...')
    _build_stl(out_dir, dist_dir, args, 'llvm-libc++')


def build_stlport(out_dir, dist_dir, args):
    print('Building stlport...')
    _build_stl(out_dir, dist_dir, args, 'stlport')


def build_platforms(out_dir, dist_dir, args):
    print('Building platforms...')
    build_args = common_build_args(out_dir, dist_dir, args)
    invoke_build('build-platforms.py', build_args)


def build_cpufeatures(_, dist_dir, __):
    path = build_support.ndk_path('sources/android/cpufeatures')
    build_support.make_package('cpufeatures', path, dist_dir)


def build_native_app_glue(_, dist_dir, __):
    path = build_support.android_path(
        'development/ndk/sources/android/native_app_glue')
    build_support.make_package('native_app_glue', path, dist_dir)


def build_ndk_helper(_, dist_dir, __):
    path = build_support.android_path(
        'development/ndk/sources/android/ndk_helper')
    build_support.make_package('ndk_helper', path, dist_dir)


def build_gtest(_, dist_dir, __):
    path = build_support.ndk_path('sources/third_party/googletest')
    build_support.make_package('gtest', path, dist_dir)


def build_build(_, dist_dir, __):
    path = build_support.ndk_path('build')
    build_support.make_package('build', path, dist_dir)


def build_python_packages(_, dist_dir, __):
    # Stage the files in a temporary directory to make things easier.
    temp_dir = tempfile.mkdtemp()
    try:
        path = os.path.join(temp_dir, 'python-packages')
        shutil.copytree(
            build_support.android_path('development/python-packages'), path)
        build_support.make_package('python-packages', path, dist_dir)
    finally:
        shutil.rmtree(temp_dir)


def build_gabixx(_out_dir, dist_dir, _args):
    print('Building gabi++...')
    path = build_support.ndk_path('sources/cxx-stl/gabi++')
    build_support.make_package('gabixx', path, dist_dir)


def build_system_stl(_out_dir, dist_dir, _args):
    print('Building system-stl...')
    path = build_support.ndk_path('sources/cxx-stl/system')
    build_support.make_package('system-stl', path, dist_dir)


def build_libandroid_support(_out_dir, dist_dir, _args):
    print('Building libandroid_support...')
    path = build_support.ndk_path('sources/android/support')
    build_support.make_package('libandroid_support', path, dist_dir)


def build_libcxxabi(_out_dir, dist_dir, _args):
    print('Building libc++abi...')
    path = build_support.ndk_path('sources/cxx-stl/llvm-libc++abi')
    build_support.make_package('libcxxabi', path, dist_dir)


def main():
    parser = ArgParser()
    args = parser.parse_args()

    if args.module is None:
        modules = ALL_MODULES
    else:
        modules = {args.module}

    if args.host_only:
        modules = {
            'clang',
            'gcc',
            'host-tools',
        }

    required_package_modules = ALL_MODULES
    if args.package and required_package_modules <= modules:
        do_package = True
    else:
        do_package = False

    # TODO(danalbert): wine?
    # We're building the Windows packages from Linux, so we can't actually run
    # any of the tests from here.
    if args.system.startswith('windows') or not do_package:
        args.test = False

    # Disable buffering on stdout so the build output doesn't hide all of our
    # "Building..." messages.
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    # Set ANDROID_BUILD_TOP.
    if 'ANDROID_BUILD_TOP' not in os.environ:
        os.environ['ANDROID_BUILD_TOP'] = os.path.realpath('..')

    out_dir = build_support.get_out_dir()
    dist_dir = build_support.get_dist_dir(out_dir)

    print('Cleaning up...')
    invoke_build('dev-cleanup.sh')

    module_builds = collections.OrderedDict([
        ('binutils', build_binutils),
        ('build', build_build),
        ('clang', build_clang),
        ('cpufeatures', build_cpufeatures),
        ('gabi++', build_gabixx),
        ('gcc', build_gcc),
        ('gcclibs', build_gcc_libs),
        ('gdbserver', build_gdbserver),
        ('gnustl', build_gnustl),
        ('gtest', build_gtest),
        ('host-tools', build_host_tools),
        ('libandroid_support', build_libandroid_support),
        ('libc++', build_libcxx),
        ('libc++abi', build_libcxxabi),
        ('native_app_glue', build_native_app_glue),
        ('ndk_helper', build_ndk_helper),
        ('platforms', build_platforms),
        ('python-packages', build_python_packages),
        ('stlport', build_stlport),
        ('system-stl', build_system_stl),
    ])

    print('Building modules: {}'.format(' '.join(modules)))
    for module in modules:
        module_builds[module](out_dir, dist_dir, args)

    if do_package:
        package_ndk(out_dir, dist_dir, args)

    if args.test:
        result = test_ndk(out_dir, args)
        sys.exit(0 if result else 1)


if __name__ == '__main__':
    main()
