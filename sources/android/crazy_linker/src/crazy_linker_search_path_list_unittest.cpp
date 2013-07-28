// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_search_path_list.h"

#include <minitest/minitest.h>
#include "crazy_linker_system_mock.h"

namespace crazy {

class TestSystem {
 public:
  TestSystem() : sys_() {
    sys_.AddRegularFile("/tmp/foo/bar", "BARBARBAR", 9);
    sys_.AddRegularFile("/tmp/zoo", "ZOO", 3);
    sys_.AddRegularFile("/foo", "Foo", 3);
    sys_.AddEnvVariable("TEST_LIBRARY_PATH", "/tmp:/");
  }

  ~TestSystem() {}

  void AddFile(const char* path, const char* data, size_t len) {
    sys_.AddRegularFile(path, data, len);
  }

 private:
  SystemMock sys_;
};

TEST(SearchPathList, Empty) {
  TestSystem sys;
  SearchPathList list;
  EXPECT_FALSE(list.FindFile("/foo"));
  EXPECT_FALSE(list.FindFile("/tmp/zoo"));
  EXPECT_FALSE(list.FindFile("/tmp/foo/bar"));
}

TEST(SearchPathList, OneItem) {
  TestSystem sys;
  SearchPathList list;
  list.AddPaths("/tmp/foo");
  EXPECT_STREQ("/tmp/foo/bar", list.FindFile("bar"));
  EXPECT_FALSE(list.FindFile("zoo"));
  EXPECT_FALSE(list.FindFile("foo"));
}

TEST(SearchPathList, Reset) {
  TestSystem sys;
  SearchPathList list;
  list.AddPaths("/tmp/foo");
  EXPECT_STREQ("/tmp/foo/bar", list.FindFile("bar"));

  list.Reset();
  EXPECT_FALSE(list.FindFile("bar"));
}

TEST(SearchPathList, ResetFromEnv) {
  TestSystem sys;
  SearchPathList list;
  list.ResetFromEnv("TEST_LIBRARY_PATH");
  EXPECT_STREQ("/tmp/foo/bar", list.FindFile("foo/bar"));
  EXPECT_STREQ("/foo", list.FindFile("foo"));
}

TEST(SearchPathList, ThreeItems) {
  TestSystem sys;
  SearchPathList list;
  list.AddPaths("/tmp/foo");
  list.AddPaths("/tmp/");

  EXPECT_STREQ("/tmp/foo/bar", list.FindFile("bar"));
  EXPECT_STREQ("/tmp/zoo", list.FindFile("zoo"));
  EXPECT_FALSE(list.FindFile("foo"));
}

TEST(SearchPathList, EnvPathsAfterAddedOnes) {
  TestSystem sys;
  sys.AddFile("/opt/foo", "FOO", 3);
  SearchPathList list;
  list.ResetFromEnv("TEST_LIBRARY_PATH");
  list.AddPaths("/opt");

  // This checks that paths added with AddPaths() have priority over
  // paths added with ResetFromEnv(). An invalid implementation would
  // find '/tmp/foo' instead.
  EXPECT_STREQ("/opt/foo", list.FindFile("foo"));
}

}  // namespace crazy