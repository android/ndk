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

import util


def format_stats_str(num_tests, stats, use_color):
    pass_label = util.color_string('PASS', 'green') if use_color else 'PASS'
    fail_label = util.color_string('FAIL', 'red') if use_color else 'FAIL'
    skip_label = util.color_string('SKIP', 'yellow') if use_color else 'SKIP'
    return '{pl} {p}/{t} {fl} {f}/{t} {sl} {s}/{t}'.format(
        pl=pass_label, p=stats['pass'],
        fl=fail_label, f=stats['fail'],
        sl=skip_label, s=stats['skip'],
        t=num_tests)


class Printer(object):
    def print_results(self, results, stats):
        raise NotImplementedError


class StdoutPrinter(Printer):
    def __init__(self, use_color=False, show_all=False):
        self.use_color = use_color
        self.show_all = show_all

    def print_results(self, results, stats):
        print()
        formatted = format_stats_str(stats.num_tests,
                                     stats.global_stats, self.use_color)
        print(formatted)
        for suite, test_results in results.items():
            stats_str = format_stats_str(len(test_results),
                                         stats.suite_stats[suite],
                                         self.use_color)
            print()
            print('{}: {}'.format(suite, stats_str))
            for result in test_results:
                if self.show_all or result.failed():
                    print(result.to_string(colored=self.use_color))
