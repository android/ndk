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
import fnmatch


class TestFilter(object):
    def __init__(self, patterns):
        self.early_filters = []
        self.late_filters = []
        for pattern in patterns:
            self.add_filter(pattern)

    def filter(self, test_name):
        filter_set = self.early_filters
        if '.' in test_name:
            filter_set = self.late_filters
        if len(filter_set) == 0:
            return True
        return any(f(test_name) for f in filter_set)

    def add_filter(self, pattern):
        """Adds a filter function based on the provided pattern.

        There are two types of filters we need to handle.

        1) Filters we can apply before running the test.
        2) Filters that cannot be run applied before running the test.

        We cannot apply all filters at a high level because we do not know all
        of the subtests until we begin running the test. The test cases for
        each device test are determined by searching for binaries in
        $OUT/libs/$ABI, and that list cannot be determined without building the
        test.

        Test filters that include a '.' will be split into two filters: one
        that filters on the whole test, and another that filters on the
        specific test case. This is done so foo will be built but bar will not
        if the case is 'foo.b*'. In this example, the resulting filters would
        be:

            Pass early filter if test name == 'foo'
            Pass late filter if case name matches 'foo.b*'

        This is still imperfect. While this does save us significant time in
        being able to skip building tests we don't care about, we have to
        specify them enough to do so. For example, *.foo will build everything.
        Also, if we have tests 'foo.bar_qux' and 'baz.bar_quux', we can't
        simply specify '*bar_*', but have to use '*.bar_*'.
        """
        # Note that the way we split the patterns does allow more than one '.'
        # to appear in the full test name. The early pattern will never contain
        # a '.', i.e. the early filter pattern for 'foo.bar.*' is 'foo'.
        early_pattern = pattern.split('.')[0]
        late_pattern = pattern
        if '.' not in pattern:
            late_pattern = pattern + '.*'

        self._add_early_filter(early_pattern)
        self._add_late_filter(late_pattern)

    def _make_filter_function(self, pattern):
        # pylint: disable=no-self-use
        def func(test_name):
            return fnmatch.fnmatch(test_name, pattern)
        return func

    def _add_early_filter(self, pattern):
        self.early_filters.append(self._make_filter_function(pattern))

    def _add_late_filter(self, pattern):
        self.late_filters.append(self._make_filter_function(pattern))

    @classmethod
    def from_string(cls, filter_string):
        return cls(filter_string.split(',') if filter_string else [])
