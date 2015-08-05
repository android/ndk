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
import unittest

from filters import TestFilter


class FilterTest(unittest.TestCase):
    def test_filters(self):
        filters = TestFilter.from_string('foo,ba*')
        self.assertTrue(filters.filter('foo'))
        self.assertTrue(filters.filter('bar'))
        self.assertTrue(filters.filter('baz'))
        self.assertFalse(filters.filter('qux'))

        filters.add_filter('woodly.*')
        filters.add_filter('doodly.b*')
        filters.add_filter('qu*.b*')

        self.assertTrue(filters.filter('foo'))
        self.assertTrue(filters.filter('foo.bar'))

        self.assertTrue(filters.filter('woodly.bar'))
        self.assertFalse(filters.filter('woo.bar'))

        self.assertTrue(filters.filter('doodly'))
        self.assertTrue(filters.filter('doodly.baz'))
        self.assertFalse(filters.filter('doodly.qux'))

        self.assertTrue(filters.filter('qux'))
        self.assertTrue(filters.filter('quux'))
        self.assertTrue(filters.filter('qux.bar'))
        self.assertTrue(filters.filter('quux.bar'))
        self.assertFalse(filters.filter('qux.foo'))
        self.assertFalse(filters.filter('qx.bar'))

    def test_empty_filters(self):
        filters = TestFilter.from_string('')
        self.assertTrue(filters.filter('foo'))
        self.assertTrue(filters.filter('foo.bar'))
