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
import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile


# TODO: Make the x86 toolchain names just be the triple.
ALL_TOOLCHAINS = (
    'arm-linux-androideabi',
    'aarch64-linux-android',
    'mipsel-linux-android',
    'mips64el-linux-android',
    'x86',
    'x86_64',
)


ALL_TRIPLES = (
    'arm-linux-androideabi',
    'aarch64-linux-android',
    'mipsel-linux-android',
    'mips64el-linux-android',
    'x86-linux-android',
    'x86_64-linux-android',
)


ALL_ARCHITECTURES = (
    'arm',
    'arm64',
    'mips',
    'mips64',
    'x86',
    'x86_64',
)


def arch_to_toolchain(arch):
    return dict(zip(ALL_ARCHITECTURES, ALL_TOOLCHAINS))[arch]


def arch_to_triple(arch):
    return dict(zip(ALL_ARCHITECTURES, ALL_TRIPLES))[arch]


def toolchain_to_arch(toolchain):
    return dict(zip(ALL_TOOLCHAINS, ALL_ARCHITECTURES))[toolchain]


def arch_to_abis(arch):
    return {
        'arm': ['armeabi', 'armeabi-v7a', 'armeabi-v7a-hard'],
        'arm64': ['arm64-v8a'],
        'mips': ['mips'],
        'mips64': ['mips64'],
        'x86': ['x86'],
        'x86_64': ['x86_64'],
    }[arch]


def abi_to_arch(arch):
    return {
        'armeabi': 'arm',
        'armeabi-v7a': 'arm',
        'armeabi-v7a-hard': 'arm',
        'arm64-v8a': 'arm64',
        'mips': 'mips',
        'mips64': 'mips64',
        'x86': 'x86',
        'x86_64': 'x86_64',
    }[arch]


def android_path(path=''):
    top = os.getenv('ANDROID_BUILD_TOP', '')
    return os.path.realpath(os.path.join(top, path))


def sysroot_path(toolchain):
    arch = toolchain_to_arch(toolchain)
    version = default_api_level(arch)

    prebuilt_ndk = 'prebuilts/ndk/current'
    sysroot_subpath = 'platforms/android-{}/arch-{}'.format(version, arch)
    return android_path(os.path.join(prebuilt_ndk, sysroot_subpath))


def ndk_path(path=''):
    return android_path(os.path.join('ndk', path))


def toolchain_path(path=''):
    return android_path(os.path.join('toolchain', path))


def default_api_level(arch):
    if '64' in arch:
        return 21
    else:
        return 9


def jobs_arg():
    return '-j{}'.format(multiprocessing.cpu_count() * 2)


def build(cmd, args):
    common_args = [
        '--verbose',
        '--package-dir={}'.format(args.package_dir),
    ]

    build_env = dict(os.environ)
    build_env['NDK_BUILDTOOLS_PATH'] = android_path('ndk/build/tools')
    build_env['ANDROID_NDK_ROOT'] = ndk_path()
    subprocess.check_call(cmd + common_args, env=build_env)


def get_default_package_dir():
    DEFAULT_OUT_DIR = android_path('out/ndk')
    out_dir = os.path.realpath(os.getenv('DIST_DIR', DEFAULT_OUT_DIR))
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)
    return out_dir


def get_default_host():
    if sys.platform in ('linux', 'linux2'):
        return 'linux'
    elif sys.platform == 'darwin':
        return 'darwin'
    else:
        raise RuntimeError('Unsupported host: {}'.format(sys.platform))


def host_to_tag(host):
    if host in ['darwin', 'linux']:
        return host + '-x86_64'
    elif host == 'windows':
        return 'windows'
    elif host == 'windows64':
        return 'windows-x86_64'
    else:
        raise RuntimeError('Unsupported host: {}'.format(host))


def make_repo_prop(out_dir):
    file_name = 'repo.prop'

    dist_dir = os.environ.get('DIST_DIR')
    if dist_dir is not None:
        dist_repo_prop = os.path.join(dist_dir, file_name)
        shutil.copy(dist_repo_prop, out_dir)
    else:
        out_file = os.path.join(out_dir, file_name)
        with open(out_file, 'w') as prop_file:
            cmd = [
                'repo', 'forall', '-c',
                'echo $REPO_PROJECT $(git rev-parse HEAD)',
            ]
            subprocess.check_call(cmd, stdout=prop_file)


def make_package(name, files, out_dir, root_dir, repo_prop_dir=''):
    """Pacakges an NDK module for release.

    Makes a tarball of the single NDK module that can be released in the SDK
    manager. This will handle the details of creating the repo.prop file for
    the package.

    Args:
        name: Name of the final package, excluding extension.
        files: List of files (relative to root_dir) to be packaged.
        out_dir: Directory to place package.
        root_dir: Directory to make package from. Equivalent to tar(1)'s -C.
    """
    path = os.path.join(out_dir, name + '.tar.bz2')

    def package_filter(path):
        ignored_extensions = (
            # Python junk.
            '.pyc',
            '.pyo',
            '.pyd',
        )

        name = os.path.basename(path)
        _, ext = os.path.splitext(name)
        return ext in ignored_extensions or name == '.gitignore'

    with tarfile.open(path, 'w:bz2') as tarball:
        for f in files:
            real_file = os.path.join(root_dir, f)
            tarball.add(real_file, f, exclude=package_filter)

        tmpdir = tempfile.mkdtemp()
        try:
            make_repo_prop(tmpdir)
            arcname = os.path.join(repo_prop_dir, 'repo.prop')
            tarball.add(os.path.join(tmpdir, 'repo.prop'), arcname)
        finally:
            shutil.rmtree(tmpdir)


class ArgParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgParser, self).__init__()

        self.add_argument(
            '--host', choices=('darwin', 'linux', 'windows', 'windows64'),
            default=get_default_host(),
            help='Build binaries for given OS (e.g. linux).')
        self.add_argument(
            '--package-dir', help='Directory to place the packaged artifact.',
            type=os.path.realpath, default=get_default_package_dir())


def run(main_func, arg_parser=ArgParser):
    if 'ANDROID_BUILD_TOP' not in os.environ:
        top = os.path.join(os.path.dirname(__file__), '../../..')
        os.environ['ANDROID_BUILD_TOP'] = os.path.realpath(top)

    args = arg_parser().parse_args()

    # We want any paths to be relative to the invoked build script.
    main_filename = os.path.realpath(sys.modules['__main__'].__file__)
    os.chdir(os.path.dirname(main_filename))

    main_func(args)
