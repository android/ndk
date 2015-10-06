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
import contextlib
import os
import subprocess


def color_string(string, color):
    colors = {
        'green': '\033[92m',
        'red': '\033[91m',
        'yellow': '\033[93m',
    }
    end_color = '\033[0m'
    return colors[color] + string + end_color


@contextlib.contextmanager
def cd(path):
    curdir = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(curdir)


def call_output(cmd, *args, **kwargs):
    """Invoke the specified command and return exit code and output.

    This is the missing subprocess.call_output, which is the combination of
    subprocess.call and subprocess.check_output. Like call, it returns an exit
    code rather than raising an exception. Like check_output, it returns the
    output of the program. Unlike check_output, it returns the output even on
    failure.

    Returns: Tuple of (exit_code, output).
    """
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, *args, **kwargs)
    out, _ = proc.communicate()
    return proc.returncode, out
