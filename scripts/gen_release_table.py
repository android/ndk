#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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
"""Generates an HTML table for the downloads page."""
from __future__ import print_function

import logging
import operator
import os.path
import re
import sys


def get_lines():
    lines = []
    while True:
        line = raw_input()
        if line.strip() == '':
            return lines
        lines.append(line)


def main():
    print('Paste the contents of the "New files" section of the SDK update '
          'email here. Terminate with an empty line.')
    lines = get_lines()
    if len(lines) == 0:
        sys.exit('No input.')

    # The user may have pasted the following header line:
    # SHA1                                              size  file
    if lines[0].startswith('SHA1'):
        lines = lines[1:]

    artifacts = []
    for line in lines:
        # Some lines are updates to the repository.xml files used by the SDK
        # manager. We don't care about these.
        # <sha>        12,345  path/to/repository.xml
        if line.endswith('.xml'):
            continue

        # Real entries look like this:
        # <sha>   123,456,789  path/to/android-ndk-r11c-linux-x86_64.zip
        match = re.match(r'^(\w+)\s+([0-9,]+)\s+(.+)$', line)
        if match is None:
            logging.error('Skipping unrecognized line: %s', line)
            continue

        sha = match.group(1)

        size_str = match.group(2)
        size = int(size_str.replace(',', ''))

        path = match.group(3)
        package = os.path.basename(path)

        # android-ndk-$VERSION-$HOST-$HOST_ARCH.$EXT
        # $VERSION might contain a hyphen for beta/RC releases.
        # Split on all hyphens and join the last two items to get the platform.
        package_name = os.path.splitext(package)[0]
        host = '-'.join(package_name.split('-')[-2:])
        pretty_host = {
            'darwin-x86_64': 'Mac OS X',
            'linux-x86_64': 'Linux',
            'windows-x86_64': 'Windows 64-bit',
            'windows-x86': 'Windows 32-bit',
        }[host]

        artifacts.append((pretty_host, package, size, sha))

    # Sort the artifacts by the platform name.
    artifacts = sorted(artifacts, key=operator.itemgetter(0))

    print('<table>')
    print('  <tr>')
    print('    <th>Platform</th>')
    print('    <th>Package</th>')
    print('    <th>Size (bytes)</th>')
    print('    <th>SHA1 Checksum</th>')
    print('  </tr>')
    for host, package, size, sha in artifacts:
        url_base = 'http://dl.google.com/android/repository/'
        package_url = url_base + package
        link = '<a href="{}">{}</a>'.format(package_url, package)

        print('  <tr>')
        print('    <td>{}</td>'.format(host))
        print('    <td>{}</td>'.format(link))
        print('    <td>{}</td>'.format(size))
        print('    <td>{}</td>'.format(sha))
        print('  </tr>')
    print('</table>')


if __name__ == '__main__':
    main()
