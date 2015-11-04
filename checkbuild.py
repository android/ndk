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

site.addsitedir(os.path.join(os.path.dirname(__file__), 'build/lib'))

import build_support  # pylint: disable=import-error


ALL_MODULES = {
    'binutils',
    'build',
    'clang',
    'cpufeatures',
    'gcc',
    'gcclibs',
    'gdbserver',
    'gnustl',
    'gtest',
    'host-tools',
    'libc++',
    'platforms',
    'stlport',
    'tests',
}


class ArgParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgParser, self).__init__(
            description=inspect.getdoc(sys.modules[__name__]))

        self.add_argument(
            '--arch',
            choices=('arm', 'arm64', 'mips', 'mips64', 'x86', 'x86_64'),
            help='Build for the given architecture. Build all by default.')

        self.add_argument(
            '--package', action='store_true', dest='package', default=True,
            help='Package the NDK when done building (default).')
        self.add_argument(
            '--no-package', action='store_false', dest='package',
            help='Do not package the NDK when done building.')

        self.add_argument(
            '--release', default=datetime.date.today().strftime('%Y%m%d'),
            help='Release name. Package will be named android-ndk-RELEASE.')

        system_group = self.add_mutually_exclusive_group()
        system_group.add_argument(
            '--system', choices=('darwin', 'linux', 'windows', 'windows64'),
            default=build_support.get_default_host(),
            help='Build for the given OS.')

        old_choices = {
            'darwin', 'darwin-x86',
            'linux', 'linux-x86',
            'windows',
        }

        system_group.add_argument(
            '--systems', choices=old_choices, dest='system',
            help='Build for the given OS. Deprecated. Use --system instead.')

        module_group = self.add_mutually_exclusive_group()

        module_group.add_argument(
            '--module', choices=ALL_MODULES, help='NDK modules to build.')

        module_group.add_argument(
            '--host-only', action='store_true',
            help='Skip building target components.')

        gcc_group = module_group.add_mutually_exclusive_group()
        gcc_group.add_argument(
            '--skip-gcc', action='store_true', default=True,
            help='Skip building and packaging GCC (default).')
        gcc_group.add_argument(
            '--no-skip-gcc', action='store_false', dest='skip_gcc',
            help='Build and package GCC.')


def _invoke_build(script, args):
    if args is None:
        args = []
    subprocess.check_call([build_support.android_path(script)] + args)


def invoke_build(script, args=None):
    script_path = os.path.join('build/tools', script)
    _invoke_build(build_support.ndk_path(script_path), args)


def invoke_external_build(script, args=None):
    _invoke_build(build_support.android_path(script), args)


def package_ndk(release_name, system, out_dir, build_args):
    package_args = [
        '--out-dir={}'.format(out_dir),
        '--prebuilt-dir={}'.format(out_dir),
        '--release={}'.format(release_name),
        '--systems={}'.format(system),
    ]
    invoke_build('package-release.sh', package_args + build_args)


def common_build_args(out_dir, args):
    build_args = ['--package-dir={}'.format(out_dir)]
    # Need to use args.system directly rather than system because system is the
    # name used by the build/tools scripts (i.e. linux-x86 instead of linux).
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


def pack_binutils(arch, host_tag, out_dir, root_dir, binutils_path):
    archive_name = '-'.join(['binutils', arch, host_tag])
    binutils_relpath = os.path.relpath(binutils_path, root_dir)
    files = [binutils_relpath]
    build_support.make_package(archive_name, files, out_dir, root_dir,
                               repo_prop_dir=binutils_relpath)


def get_prebuilt_gcc(host, arch):
    tag = build_support.host_to_tag(host)
    system_subdir = 'prebuilts/ndk/current/toolchains/{}'.format(tag)
    system_path = build_support.android_path(system_subdir)
    toolchain = build_support.arch_to_toolchain(arch)
    toolchain_dir = toolchain + '-4.9'
    return os.path.join(system_path, toolchain_dir, 'prebuilt')


def build_binutils(out_dir, args):
    print('Extracting binutils package from GCC...')

    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    host_tag = build_support.host_to_tag(args.system)

    for arch in arches:
        toolchain = build_support.arch_to_toolchain(arch)
        toolchain_path = get_prebuilt_gcc(args.system, arch)

        triple = fixup_toolchain_triple(toolchain)
        tmpdir = tempfile.mkdtemp()
        try:
            install_dir = os.path.join(tmpdir, 'binutils', host_tag,
                                       toolchain)
            os.makedirs(install_dir)
            is_windows = host_tag.startswith('windows')
            has_gold = not triple.startswith('mips') and not is_windows
            for file_name in get_binutils_files(triple, has_gold, is_windows):
                install_file(file_name, toolchain_path, install_dir)
            pack_binutils(arch, host_tag, out_dir, tmpdir, install_dir)
        finally:
            shutil.rmtree(tmpdir)


def build_clang(out_dir, args):
    print('Building Clang...')
    invoke_build('build-llvm.py', common_build_args(out_dir, args))


def build_gcc(out_dir, args):
    gcc_build_args = common_build_args(out_dir, args)
    if args.arch is not None:
        toolchain_name = build_support.arch_to_toolchain(args.arch)
        gcc_build_args.append('--toolchain={}'.format(toolchain_name))
    print('Building GCC...')
    invoke_external_build('toolchain/gcc/build.py', gcc_build_args)


def build_gcc_libs(out_dir, args):
    print('Packaging GCC libs...')

    arches = build_support.ALL_ARCHITECTURES
    if args.arch is not None:
        arches = [args.arch]

    for arch in arches:
        toolchain = build_support.arch_to_toolchain(arch)
        triple = fixup_toolchain_triple(toolchain)
        libgcc_subdir = 'lib/gcc/{}/4.9.x-google'.format(triple)
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

        tmpdir = tempfile.mkdtemp()
        try:
            install_dir = os.path.join(tmpdir, 'prebuilt/gcclibs', triple)
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

                archive_name = os.path.join(out_dir, 'gcclibs-' + arch)
                gcclibs_relpath = os.path.relpath(install_dir, tmpdir)
                files = [gcclibs_relpath]
                root_dir = os.path.realpath(tmpdir)
                build_support.make_package(archive_name, files, out_dir,
                                           root_dir,
                                           repo_prop_dir=gcclibs_relpath)
        finally:
            shutil.rmtree(tmpdir)


def build_host_tools(out_dir, args):
    build_args = common_build_args(out_dir, args)

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


def build_gdbserver(out_dir, args):
    print('Building gdbserver...')
    build_args = common_build_args(out_dir, args)
    if args.arch is not None:
        build_args.append('--arch={}'.format(args.arch))
    invoke_build('build-gdbserver.py', build_args)


def _build_stl(out_dir, args, stl):
    build_args = common_build_args(out_dir, args)
    if args.arch is not None:
        build_args.append('--arch={}'.format(args.arch))
    script = 'ndk/sources/cxx-stl/{}/build.py'.format(stl)
    invoke_external_build(script, build_args)


def build_gnustl(out_dir, args):
    print('Building gnustl...')
    _build_stl(out_dir, args, 'gnu-libstdc++')


def build_libcxx(out_dir, args):
    print('Building libc++...')
    _build_stl(out_dir, args, 'llvm-libc++')


def build_stlport(out_dir, args):
    print('Building stlport...')
    _build_stl(out_dir, args, 'stlport')


def build_platforms(out_dir, args):
    print('Building platforms...')
    invoke_build('build-platforms.py', common_build_args(out_dir, args))


def build_tests(out_dir, _):
    root_dir = build_support.ndk_path()
    path = 'tests'
    build_support.make_package('tests', [path], out_dir, root_dir,
                               repo_prop_dir=path)


def build_cpufeatures(out_dir, _):
    root_dir = build_support.ndk_path()
    path = 'sources/android/cpufeatures'
    build_support.make_package('cpufeatures', [path], out_dir, root_dir,
                               repo_prop_dir=path)


def build_gtest(out_dir, _):
    root_dir = build_support.ndk_path()
    path = 'sources/third_party/googletest'
    build_support.make_package('gtest', [path], out_dir, root_dir,
                               repo_prop_dir=path)


def build_build(out_dir, _):
    root_dir = build_support.ndk_path()
    files = [
        'build',
        'ndk-build',
        'ndk-build.cmd',
        'realpath',
    ]
    build_support.make_package('build', files, out_dir, root_dir,
                               repo_prop_dir='build')


def main():
    args, package_args = ArgParser().parse_known_args()

    # Disable buffering on stdout so the build output doesn't hide all of our
    # "Building..." messages.
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

    os.chdir(os.path.dirname(os.path.realpath(__file__)))

    # Set ANDROID_BUILD_TOP.
    if 'ANDROID_BUILD_TOP' not in os.environ:
        os.environ['ANDROID_BUILD_TOP'] = os.path.realpath('..')
    build_top = os.getenv('ANDROID_BUILD_TOP')

    system = args.system
    if system != 'windows':
        package_args.append('--try-64')

    # TODO(danalbert): Update build server to pass just 'linux'.
    original_system = system
    if system == 'darwin':
        system = 'darwin-x86'
    elif system == 'linux':
        system = 'linux-x86'
    elif system == 'windows64':
        system = 'windows'

    if system not in ('darwin-x86', 'linux-x86', 'windows'):
        sys.exit('Unknown system requested: {}'.format(original_system))

    package_args.append('--systems={}'.format(system))

    package_args.append(os.path.join(build_top, 'toolchain'))

    DEFAULT_OUT_DIR = os.path.join(build_top, 'out/ndk')
    out_dir = os.path.realpath(os.getenv('DIST_DIR', DEFAULT_OUT_DIR))
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    if args.module is None:
        modules = ALL_MODULES - {'gcc'}
    else:
        modules = {args.module}

    if args.host_only:
        modules = {
            'clang',
            'host-tools',
        }

    if not args.skip_gcc:
        modules |= {'gcc'}

    if args.arch is not None:
        package_args.append('--arch={}'.format(args.arch))

    print('Cleaning up...')
    invoke_build('dev-cleanup.sh')

    module_builds = collections.OrderedDict([
        ('binutils', build_binutils),
        ('build', build_build),
        ('clang', build_clang),
        ('cpufeatures', build_cpufeatures),
        ('gcc', build_gcc),
        ('gcclibs', build_gcc_libs),
        ('gdbserver', build_gdbserver),
        ('gnustl', build_gnustl),
        ('gtest', build_gtest),
        ('host-tools', build_host_tools),
        ('libc++', build_libcxx),
        ('platforms', build_platforms),
        ('stlport', build_stlport),
        ('tests', build_tests),
    ])

    print('Building modules: {}'.format(' '.join(modules)))
    for module in modules:
        module_builds[module](out_dir, args)

    required_package_modules = ALL_MODULES - {'gcc'}
    if args.package and required_package_modules <= modules:
        package_ndk(args.release, system, out_dir, package_args)


if __name__ == '__main__':
    main()
