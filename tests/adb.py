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
"""ADB handling for NDK tests."""
import os
import re
import subprocess


def push(src, dst):
    with open(os.devnull, 'wb') as dev_null:
        subprocess.check_call(['adb', 'push', src, dst], stdout=dev_null,
                              stderr=dev_null)


def shell(command):
    # Work around the fact that adb doesn't return shell exit status.
    p = subprocess.Popen(['adb', 'shell', command + '; echo $?'],
                         stdout=subprocess.PIPE)
    out, _ = p.communicate()
    if p.returncode != 0:
        raise RuntimeError('adb shell failed')

    out = re.split(r'[\r\n]+', out)
    if out[-1] == '':
        # Splitting 'foo\n' will return ['foo', '']. Lose the last element.
        out = out[:-1]
    result = int(out[-1])
    out = out[:-1]
    return result, out


def get_prop(prop_name):
    result, output = shell('getprop {}'.format(prop_name))
    if result != 0:
        raise RuntimeError('getprop failed:\n' + '\n'.join(output))
    if len(output) != 1:
        raise RuntimeError('Too many lines in getprop output:\n' +
                           '\n'.join(output))
    value = output[0]
    if not value.strip():
        return None
    return value
