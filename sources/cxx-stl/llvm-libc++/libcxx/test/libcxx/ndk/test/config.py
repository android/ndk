import os

import libcxx.test.config
import libcxx.test.target_info
import libcxx.android.build
import libcxx.android.test.format


class AndroidTargetInfo(libcxx.test.target_info.TargetInfo):
    def platform(self):
        return 'android'

    def system(self):
        raise NotImplementedError

    def platform_ver(self):
        raise NotImplementedError

    def platform_name(self):
        raise NotImplementedError

    def supports_locale(self, loc):
        raise NotImplementedError


class Configuration(libcxx.test.config.Configuration):
    def __init__(self, lit_config, config):
        super(Configuration, self).__init__(lit_config, config)
        self.cxx_under_test = None
        self.build_cmds_dir = None
        self.cxx_template = None
        self.link_template = None

    def configure(self):
        self.configure_target_info()
        self.configure_cxx()
        self.configure_triple()
        self.configure_src_root()
        self.configure_obj_root()
        self.configure_cxx_library_root()
        self.configure_compile_flags()
        self.configure_link_flags()
        self.configure_features()

    def configure_target_info(self):
        self.target_info = AndroidTargetInfo()

    def configure_compile_flags(self):
        super(Configuration, self).configure_compile_flags()

        android_support_headers = os.path.join(
            os.environ['NDK'], 'sources/android/support/include')
        self.cxx.compile_flags.append('-I' + android_support_headers)

    def configure_link_flags(self):
        self.cxx.link_flags.append('-nodefaultlibs')

        # Configure libc++ library paths.
        self.cxx.link_flags.append('-L' + self.cxx_library_root)

        gcc_toolchain = self.get_lit_conf('gcc_toolchain')
        self.cxx.link_flags.append('-gcc-toolchain')
        self.cxx.link_flags.append(gcc_toolchain)

        triple = self.get_lit_conf('target_triple')
        if triple.startswith('arm-'):
            self.cxx.link_flags.append('-lunwind')
            self.cxx.link_flags.append('-latomic')

        self.cxx.link_flags.append('-lgcc')

        self.cxx.link_flags.append('-lc++_shared')
        self.cxx.link_flags.append('-lc')
        self.cxx.link_flags.append('-lm')
        self.cxx.link_flags.append('-ldl')
        self.cxx.link_flags.append('-pie')

    def configure_features(self):
        self.config.available_features.add('c++11')
        self.config.available_features.add('long_tests')

    def get_test_format(self):
        tmp_dir = getattr(self.config, 'device_dir', '/data/local/tmp/')

        return libcxx.android.test.format.TestFormat(
            self.cxx,
            self.libcxx_src_root,
            self.libcxx_obj_root,
            tmp_dir,
            getattr(self.config, 'timeout', '300'),
            exec_env={'LD_LIBRARY_PATH': tmp_dir})
