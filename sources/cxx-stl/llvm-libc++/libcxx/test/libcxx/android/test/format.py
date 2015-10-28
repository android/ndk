import os

import lit.util  # pylint: disable=import-error

from libcxx.android.executors import AdbExecutor
from libcxx.test.executor import LocalExecutor, TimeoutExecutor
import libcxx.test.format
import libcxx.android.adb as adb


class HostTestFormat(libcxx.test.format.LibcxxTestFormat):
    # pylint: disable=super-init-not-called
    def __init__(self, cxx, libcxx_src_root, libcxx_obj_root, timeout,
                 exec_env=None):
        self.cxx = cxx
        self.libcxx_src_root = libcxx_src_root
        self.libcxx_obj_root = libcxx_obj_root
        self.use_verify_for_fail = False
        self.executor = TimeoutExecutor(timeout, LocalExecutor())

        # We need to use LD_LIBRARY_PATH because the build system's rpath is
        # relative, which won't work since we're running from /tmp. We can
        # either scan `cxx_under_test`/`link_template` to determine whether
        # we're 32-bit or 64-bit, scan testconfig.mk, or just add both
        # directories and let the linker sort it out. I'm choosing the lazy
        # option.
        outdir = os.getenv('ANDROID_HOST_OUT')
        libpath = os.pathsep.join([
            os.path.join(outdir, 'lib'),
            os.path.join(outdir, 'lib64'),
        ])
        default_env = {'LD_LIBRARY_PATH': libpath}
        self.exec_env = default_env if exec_env is None else exec_env


class TestFormat(HostTestFormat):
    def __init__(self, cxx, libcxx_src_root, libcxx_obj_root, device_dir,
                 timeout, exec_env=None):
        HostTestFormat.__init__(
            self,
            cxx,
            libcxx_src_root,
            libcxx_obj_root,
            timeout,
            exec_env)
        self.device_dir = device_dir
        self.executor = TimeoutExecutor(timeout, AdbExecutor())

    def _working_directory(self, file_name):
        return os.path.join(self.device_dir, file_name)

    def _wd_path(self, test_name, file_name):
        return os.path.join(self._working_directory(test_name), file_name)

    def _build(self, exec_path, source_path, compile_only=False,
               use_verify=False):
        # pylint: disable=protected-access
        cmd, report, rc = libcxx.test.format.LibcxxTestFormat._build(
            self, exec_path, source_path, compile_only, use_verify)
        if rc != 0:
            return cmd, report, rc

        try:
            exec_file = os.path.basename(exec_path)

            adb.mkdir(self._working_directory(exec_file))
            adb.push(exec_path, self._wd_path(exec_file, exec_file))

            # Push any .dat files in the same directory as the source to the
            # working directory.
            src_dir = os.path.dirname(source_path)
            data_files = [f for f in os.listdir(src_dir) if f.endswith('.dat')]
            for data_file in data_files:
                df_path = os.path.join(src_dir, data_file)
                df_dev_path = self._wd_path(exec_file, data_file)
                adb.push(df_path, df_dev_path)
            return cmd, report, rc
        except adb.AdbError as ex:
            return self._make_report(ex.cmd, ex.out, ex.err, ex.exit_code)

    def _clean(self, exec_path):
        exec_file = os.path.basename(exec_path)
        cmd = ['adb', 'shell', 'rm', '-rf', self._working_directory(exec_file)]
        lit.util.executeCommand(cmd)
        try:
            os.remove(exec_path)
        except OSError:
            pass

    def _run(self, exec_path, _, in_dir=None):
        raise NotImplementedError()
